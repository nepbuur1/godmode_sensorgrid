// by Marius Versteegen, 2025
// Header-only ADS1220 24-bit ADC driver via SPI + MCP23017 chip select.
// Adapted from ScoliosePCB2/ADS1220 driver for ESP32-S3.

#pragma once
#include <Arduino.h>
#include <SPI.h>
#include "crt_MCP23017.h"

namespace crt
{
	// ADS1220 SPI Commands
	static const uint8_t ADS_CMD_RESET = 0x06;
	static const uint8_t ADS_CMD_START = 0x08;
	static const uint8_t ADS_CMD_RDATA = 0x10;
	static const uint8_t ADS_CMD_RREG  = 0x20;
	static const uint8_t ADS_CMD_WREG  = 0x40;

	// Turbo mode SPS selection
	enum ADS1220TurboSps
	{
		TURBO_40_SPS   = 0,
		TURBO_90_SPS   = 1,
		TURBO_180_SPS  = 2,
		TURBO_350_SPS  = 3,
		TURBO_660_SPS  = 4,
		TURBO_1200_SPS = 5,
		TURBO_2000_SPS = 6
	};

	struct ADS1220TurboOption
	{
		uint8_t  reg1;
		uint16_t turboSps;
		uint32_t tClkCycles;
	};

	static const ADS1220TurboOption ADS1220_TURBO_OPTIONS[] = {
		{ 0x10,   40, 102434 },
		{ 0x30,   90,  45618 },
		{ 0x50,  180,  23122 },
		{ 0x70,  350,  11890 },
		{ 0x90,  660,   6290 },
		{ 0xB0, 1200,   3506 },
		{ 0xD0, 2000,   2130 }
	};

	// Internal oscillator frequency (Hz)
	static const uint32_t ADS_FCLK_HZ = 4096000;
	// Oscillator wakeup time in turbo single-shot mode (us)
	static const uint32_t ADS_OSC_WAKEUP_US = 25;

	class ADS1220
	{
	private:
		McpPin _csPin;
		MCP23017& _mcp;
		SPIClass* _spi;

		void select()   { _mcp.digitalWrite(_csPin, LOW);  }
		void deselect() { _mcp.digitalWrite(_csPin, HIGH); }

	public:
		ADS1220(McpPin csPin, MCP23017& mcp, SPIClass* spi = &SPI)
			: _csPin(csPin), _mcp(mcp), _spi(spi) {}

		bool begin(uint8_t reg1)
		{
			_mcp.pinMode(_csPin, OUTPUT);
			deselect();

			select();
			_spi->beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));

			// Reset
			_spi->transfer(ADS_CMD_RESET);
			delayMicroseconds(50);

			// Register 0: AIN1(+) - AIN0(-), Gain 1, PGA bypass -> 0x61
			_spi->transfer(ADS_CMD_WREG | (0x00 << 2));
			_spi->transfer(0x61);

			// Register 1: caller-selected data rate / mode / conversion mode
			_spi->transfer(ADS_CMD_WREG | (0x01 << 2));
			_spi->transfer(reg1);

			// Register 2: External reference on REFP0/REFN0 (2.5 V)
			_spi->transfer(ADS_CMD_WREG | (0x02 << 2));
			_spi->transfer(0x40);

			// Diagnostic readback
			_spi->transfer(ADS_CMD_RREG | (0x00 << 2));
			uint8_t reg0 = _spi->transfer(0x00);

			_spi->transfer(ADS_CMD_RREG | (0x02 << 2));
			uint8_t reg2 = _spi->transfer(0x00);

			_spi->endTransaction();
			deselect();

			bool ok = (reg0 == 0x31 || reg0 == 0x61) && (reg2 == 0x40);

			ESP_LOGI("ADS1220", "CS=%u: MUX=0x%02X %s, VREF=0x%02X %s",
					 static_cast<uint8_t>(_csPin),
					 reg0, (reg0 == 0x31 || reg0 == 0x61) ? "OK" : "FAIL",
					 reg2, (reg2 == 0x40) ? "OK" : "FAIL");

			return ok;
		}

		void startConversion()
		{
			select();
			_spi->beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));
			_spi->transfer(ADS_CMD_START);
			_spi->endTransaction();
			deselect();
		}

		int32_t readRaw()
		{
			select();
			_spi->beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));

			_spi->transfer(ADS_CMD_RDATA);
			uint8_t msb = _spi->transfer(0x00);
			uint8_t mid = _spi->transfer(0x00);
			uint8_t lsb = _spi->transfer(0x00);

			_spi->endTransaction();
			deselect();

			int32_t result = ((uint32_t)msb << 16) | ((uint32_t)mid << 8) | lsb;
			if (result & 0x800000) result |= 0xFF000000;

			return result;
		}

		static uint16_t rawToUint16(int32_t raw)
		{
			if (raw < 0) raw = 0;
			if (raw > 8388607L) raw = 8388607L;
			return (uint16_t)(((float)raw / 8388607.0f) * 65535.0f);
		}
	}; // end class ADS1220

} // end namespace crt
