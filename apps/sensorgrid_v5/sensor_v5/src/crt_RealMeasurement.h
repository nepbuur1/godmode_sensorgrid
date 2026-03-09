// by Marius Versteegen, 2025
// Real measurement provider using MCP23017 + ADG706 mux + ADS1220 ADCs.
// Reads an 8-row x 16-column pressure sensor grid and transposes the
// data before placing it in the output buffer (cols become rows).
// Adapted from ScoliosePCB2 measurement loop.

#pragma once
#include <Arduino.h>
#include <inttypes.h>
#include <SPI.h>
#include <Wire.h>
#include "crt_IMeasurementProvider.h"
#include "crt_MCP23017.h"
#include "crt_ADG706.h"
#include "crt_ADS1220.h"

namespace crt
{
	class RealMeasurement : public IMeasurementProvider
	{
	private:
		// Pin definitions (matching ScoliosePCB2 hardware)
		static const int I2C_SDA_PIN = 4;
		static const int I2C_SCL_PIN = 5;
		static const int SPI_SCLK_PIN = 12;
		static const int SPI_MISO_PIN = 13;
		static const int SPI_MOSI_PIN = 11;
		static const int SPI_CS_HOST_PIN = 10;
		static const int MCP_RESET_PIN = 14;
		static const uint8_t MCP_ADDR = 0x24;

		static const int NUM_ROWS = 8;
		static const int NUM_COLS = 16;

		// Timing parameters
		static const uint32_t T_LEAD_US = 700;
		static constexpr ADS1220TurboSps TURBO_SPS = TURBO_2000_SPS;
		static constexpr float SAFETY_MARGIN = 0.1f;
		static constexpr float FILTER_FACTOR = 0.5f;

		// CS pin mapping: 8 ADCs (rows 0-7) via MCP23017
		static constexpr McpPin ADC_CS_PINS[NUM_ROWS] = {
			McpPin::GPA2, McpPin::GPA3, McpPin::GPA4, McpPin::GPA5,
			McpPin::GPA6, McpPin::GPA7, McpPin::GPB0, McpPin::GPB1
		};

		// Port masks for batch CS operations
		static const uint8_t ADC_CS_MASK_A = 0xFC; // Port A bits 2-7
		static const uint8_t ADC_CS_MASK_B = 0x03; // Port B bits 0-1

		// Hardware objects
		MCP23017 mcp;
		ADG706 colMux;
		ADS1220 adc0, adc1, adc2, adc3, adc4, adc5, adc6, adc7;
		ADS1220* adcs[NUM_ROWS];

		// Internal storage: row-major [row][col]
		uint16_t sensorValues[NUM_ROWS][NUM_COLS];

		// Timing (computed at init from SPS selection)
		uint32_t tConvUs;
		uint32_t tMeasTimeUs;

		void computeTimingParameters()
		{
			const float tI2cRead = 96.0f, tI2cWrite = 71.0f;
			const float tI2cRmw = tI2cRead + tI2cWrite;
			const float tStep3 = 4 * tI2cRmw + 8;
			const float tDeselectOverlap = 2 * tI2cRmw;

			int numDeselects = 1;
			for (int i = 0; i + 1 < NUM_ROWS; i++)
			{
				bool curA = (static_cast<uint8_t>(ADC_CS_PINS[i]) < 8);
				bool nextA = (static_cast<uint8_t>(ADC_CS_PINS[i + 1]) < 8);
				if (curA != nextA) numDeselects++;
			}
			int numNoDeselect = NUM_ROWS - numDeselects;

			const float tSpiRead = 32.0f;
			const float tStep5 = 2 * tI2cRead
				+ numNoDeselect * (tI2cWrite + tSpiRead)
				+ numDeselects * (tI2cWrite + tSpiRead + tI2cWrite);

			const ADS1220TurboOption& opt = ADS1220_TURBO_OPTIONS[TURBO_SPS];
			const float sfFactor = 1.0f + SAFETY_MARGIN;
			float tConvRaw = (float)opt.tClkCycles / (ADS_FCLK_HZ / 1000000.0f)
				+ ADS_OSC_WAKEUP_US;
			tConvUs = (uint32_t)(tConvRaw * sfFactor + 0.5f);

			float tStep4 = tConvUs - tDeselectOverlap;
			if (tStep4 < 0) tStep4 = 0;
			tMeasTimeUs = (uint32_t)(tStep3 + tStep4 + tStep5 + 0.5f);
		}

		void selectAllAdcs()
		{
			uint8_t portA = mcp.readReg(MCP_GPIOA);
			mcp.writeReg(MCP_OLATA, portA & ~ADC_CS_MASK_A);
			uint8_t portB = mcp.readReg(MCP_GPIOB);
			mcp.writeReg(MCP_OLATB, portB & ~ADC_CS_MASK_B);
		}

		void deselectAllAdcs()
		{
			uint8_t portA = mcp.readReg(MCP_GPIOA);
			mcp.writeReg(MCP_OLATA, portA | ADC_CS_MASK_A);
			uint8_t portB = mcp.readReg(MCP_GPIOB);
			mcp.writeReg(MCP_OLATB, portB | ADC_CS_MASK_B);
		}

		void startAllConversions()
		{
			selectAllAdcs();
			SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));
			SPI.transfer(ADS_CMD_START);
			SPI.endTransaction();
			deselectAllAdcs();
		}

		// Phase 4b optimized readback: merged select/deselect for same-port transitions
		void readAllAdcsFast(int col)
		{
			uint8_t portA_allDesel = mcp.readReg(MCP_GPIOA) | ADC_CS_MASK_A;
			uint8_t portB_allDesel = mcp.readReg(MCP_GPIOB) | ADC_CS_MASK_B;

			SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));

			for (int i = 0; i < NUM_ROWS; i++)
			{
				uint8_t csPin = static_cast<uint8_t>(ADC_CS_PINS[i]);
				bool onPortA = (csPin < 8);

				if (onPortA)
					mcp.writeReg(MCP_OLATA, portA_allDesel & ~(1 << csPin));
				else
					mcp.writeReg(MCP_OLATB, portB_allDesel & ~(1 << (csPin - 8)));

				SPI.transfer(ADS_CMD_RDATA);
				uint8_t msb = SPI.transfer(0x00);
				uint8_t mid = SPI.transfer(0x00);
				uint8_t lsb = SPI.transfer(0x00);

				bool needDeselect;
				if (i + 1 >= NUM_ROWS)
				{
					needDeselect = true;
				}
				else
				{
					bool nextOnPortA = (static_cast<uint8_t>(ADC_CS_PINS[i + 1]) < 8);
					needDeselect = (onPortA != nextOnPortA);
				}

				if (needDeselect)
				{
					if (onPortA)
						mcp.writeReg(MCP_OLATA, portA_allDesel);
					else
						mcp.writeReg(MCP_OLATB, portB_allDesel);
				}

				int32_t result = ((uint32_t)msb << 16) | ((uint32_t)mid << 8) | lsb;
				if (result & 0x800000) result |= 0xFF000000;
				uint16_t measured = ADS1220::rawToUint16(result);

				// EMA filter
				sensorValues[i][col] = (uint16_t)(
					sensorValues[i][col] * FILTER_FACTOR
					+ measured * (1.0f - FILTER_FACTOR) + 0.5f);
			}

			SPI.endTransaction();
		}

	public:
		RealMeasurement()
			: mcp(MCP_ADDR), colMux(mcp),
			  adc0(ADC_CS_PINS[0], mcp), adc1(ADC_CS_PINS[1], mcp),
			  adc2(ADC_CS_PINS[2], mcp), adc3(ADC_CS_PINS[3], mcp),
			  adc4(ADC_CS_PINS[4], mcp), adc5(ADC_CS_PINS[5], mcp),
			  adc6(ADC_CS_PINS[6], mcp), adc7(ADC_CS_PINS[7], mcp),
			  tConvUs(0), tMeasTimeUs(0)
		{
			adcs[0] = &adc0; adcs[1] = &adc1; adcs[2] = &adc2; adcs[3] = &adc3;
			adcs[4] = &adc4; adcs[5] = &adc5; adcs[6] = &adc6; adcs[7] = &adc7;
			memset(sensorValues, 0, sizeof(sensorValues));
		}

		bool init() override
		{
			// Reset MCP23017
			::pinMode(MCP_RESET_PIN, OUTPUT);
			::digitalWrite(MCP_RESET_PIN, LOW);
			delay(10);
			::digitalWrite(MCP_RESET_PIN, HIGH);
			delay(10);

			// Init MCP23017 on I2C
			if (!mcp.begin(I2C_SDA_PIN, I2C_SCL_PIN, 400000))
			{
				ESP_LOGE("RealMeasurement", "MCP23017 init failed");
				return false;
			}
			Wire.setClock(400000);

			// Init SPI
			::pinMode(SPI_CS_HOST_PIN, OUTPUT);
			::digitalWrite(SPI_CS_HOST_PIN, HIGH);
			SPI.begin(SPI_SCLK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, SPI_CS_HOST_PIN);

			// Init column multiplexer
			colMux.begin();

			// Compute timing parameters
			computeTimingParameters();

			// Init all 8 ADCs
			const ADS1220TurboOption& opt = ADS1220_TURBO_OPTIONS[TURBO_SPS];
			for (int i = 0; i < NUM_ROWS; i++)
			{
				adcs[i]->begin(opt.reg1);
				delay(10);
			}

			ESP_LOGI("RealMeasurement", "Real mode: %dx%d, tConv=%" PRIu32 " us, tMeas=%" PRIu32 " us",
					 NUM_ROWS, NUM_COLS, tConvUs, tMeasTimeUs);
			return true;
		}

		void measure(uint16_t* buffer) override
		{
			// Full scan: iterate all 16 columns
			for (int col = 0; col < NUM_COLS; col++)
			{
				uint32_t colStartUs = micros();

				// Step 1: Select column via mux
				colMux.selectColumn(col);

				// Step 2: Wait for analog settling
				while ((micros() - colStartUs) < T_LEAD_US) {}

				// Step 3: Start all ADC conversions simultaneously
				startAllConversions();

				// Step 4: Wait for conversion to complete
				uint32_t convStartUs = micros();
				while ((micros() - convStartUs) < tConvUs) {}

				// Step 5: Read all ADC values
				readAllAdcsFast(col);

				// Pad remainder of cycle
				while ((micros() - colStartUs) < (T_LEAD_US + tMeasTimeUs)) {}
			}

			// Transpose + invert: sensorValues[row][col] -> buffer in
			// reversed col-major order to mirror grid in both X and Y,
			// matching physical pressure-sensor geometry.
			uint16_t idx = 0;
			for (int col = NUM_COLS - 1; col >= 0; col--)
			{
				for (int row = NUM_ROWS - 1; row >= 0; row--)
				{
					buffer[idx++] = sensorValues[row][col];
				}
			}
		}
	}; // end class RealMeasurement

	constexpr McpPin RealMeasurement::ADC_CS_PINS[RealMeasurement::NUM_ROWS];

} // end namespace crt
