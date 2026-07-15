// by Marius Versteegen, 2025
// Header-only MCP23017 I2C GPIO expander driver.
// Adapted from 7Semi_MCP23017 driver for ESP32-S3.

#pragma once
#include <Arduino.h>
#include <Wire.h>

namespace crt
{
	// MCP23017 Register Addresses
	static const uint8_t MCP_IODIRA  = 0x00;
	static const uint8_t MCP_IODIRB  = 0x01;
	static const uint8_t MCP_IPOLA   = 0x02;
	static const uint8_t MCP_IPOLB   = 0x03;
	static const uint8_t MCP_GPPUA   = 0x0C;
	static const uint8_t MCP_GPPUB   = 0x0D;
	static const uint8_t MCP_INTFA   = 0x0E;
	static const uint8_t MCP_INTFB   = 0x0F;
	static const uint8_t MCP_INTCAPA = 0x10;
	static const uint8_t MCP_INTCAPB = 0x11;
	static const uint8_t MCP_GPIOA   = 0x12;
	static const uint8_t MCP_GPIOB   = 0x13;
	static const uint8_t MCP_OLATA   = 0x14;
	static const uint8_t MCP_OLATB   = 0x15;

	enum class McpPin : uint8_t
	{
		GPA0 = 0, GPA1, GPA2, GPA3, GPA4, GPA5, GPA6, GPA7,
		GPB0, GPB1, GPB2, GPB3, GPB4, GPB5, GPB6, GPB7
	};

	class MCP23017
	{
	private:
		uint8_t _address;
		TwoWire* _i2c;

	public:
		MCP23017(uint8_t address = 0x20, TwoWire* i2c = &Wire)
			: _address(address), _i2c(i2c) {}

		// Probe I2C bus for MCP23017 without full initialization.
		static bool detect(uint8_t address, TwoWire* i2c, int sda, int scl)
		{
			i2c->begin(sda, scl, 400000);
			i2c->beginTransmission(address);
			bool found = (i2c->endTransmission() == 0);
			i2c->end();
			return found;
		}

		bool begin(int sda, int scl, uint32_t freq = 400000)
		{
			_i2c->begin(sda, scl, freq);

			_i2c->beginTransmission(_address);
			if (_i2c->endTransmission() != 0) return false;

			if (!writeReg(MCP_IODIRA, 0x00)) return false;
			if (!writeReg(MCP_IODIRB, 0x00)) return false;
			if (!writeReg(MCP_GPPUA, 0x00)) return false;
			if (!writeReg(MCP_GPPUB, 0x00)) return false;

			return true;
		}

		void pinMode(McpPin pin, uint8_t mode)
		{
			uint8_t p = static_cast<uint8_t>(pin);
			if (p < 8)
			{
				uint8_t iodir = readReg(MCP_IODIRA);
				if (mode == INPUT) iodir |= (1 << p);
				else iodir &= ~(1 << p);
				writeReg(MCP_IODIRA, iodir);
			}
			else
			{
				uint8_t iodir = readReg(MCP_IODIRB);
				if (mode == INPUT) iodir |= (1 << (p - 8));
				else iodir &= ~(1 << (p - 8));
				writeReg(MCP_IODIRB, iodir);
			}
		}

		void digitalWrite(McpPin pin, bool value)
		{
			uint8_t p = static_cast<uint8_t>(pin);
			if (p < 8)
			{
				uint8_t state = readReg(MCP_GPIOA);
				if (value) state |= (1 << p);
				else state &= ~(1 << p);
				writeReg(MCP_OLATA, state);
			}
			else
			{
				uint8_t state = readReg(MCP_GPIOB);
				if (value) state |= (1 << (p - 8));
				else state &= ~(1 << (p - 8));
				writeReg(MCP_OLATB, state);
			}
		}

		uint8_t digitalRead(McpPin pin)
		{
			uint8_t p = static_cast<uint8_t>(pin);
			if (p < 8)
			{
				return (readReg(MCP_GPIOA) & (1 << p)) ? HIGH : LOW;
			}
			else
			{
				return (readReg(MCP_GPIOB) & (1 << (p - 8))) ? HIGH : LOW;
			}
		}

		bool writeReg(uint8_t reg, uint8_t value)
		{
			_i2c->beginTransmission(_address);
			_i2c->write(reg);
			_i2c->write(value);
			return (_i2c->endTransmission() == 0);
		}

		uint8_t readReg(uint8_t reg)
		{
			_i2c->beginTransmission(_address);
			_i2c->write(reg);
			_i2c->endTransmission();
			_i2c->requestFrom(_address, (uint8_t)1);
			return _i2c->read();
		}
	}; // end class MCP23017

} // end namespace crt
