// by Marius Versteegen, 2025
// Header-only ADG706 16-channel multiplexer driver.
// Controlled via MCP23017 GPIO expander.
// Adapted from ScoliosePCB2/ADG706 driver for ESP32-S3.

#pragma once
#include "crt_MCP23017.h"

namespace crt
{
	class ADG706
	{
	private:
		MCP23017& _mcp;

		static constexpr McpPin PIN_EN = McpPin::GPB5;
		static constexpr McpPin PIN_A0 = McpPin::GPB2;
		static constexpr McpPin PIN_A1 = McpPin::GPB3;
		static constexpr McpPin PIN_A2 = McpPin::GPB4;
		static constexpr McpPin PIN_A3 = McpPin::GPB6;

		// Maps logical column index (0..15) to ADG706 S-channel address
		static constexpr uint8_t COL_TO_ADDR[16] = {
			7, 6, 5, 4, 3, 2, 1, 0,       // cols 0-7
			8, 15, 14, 13, 12, 11, 10, 9   // cols 8-15
		};

	public:
		ADG706(MCP23017& mcp) : _mcp(mcp) {}

		void begin()
		{
			_mcp.pinMode(PIN_EN, OUTPUT);
			_mcp.pinMode(PIN_A0, OUTPUT);
			_mcp.pinMode(PIN_A1, OUTPUT);
			_mcp.pinMode(PIN_A2, OUTPUT);
			_mcp.pinMode(PIN_A3, OUTPUT);

			disable();
		}

		void selectColumn(uint8_t col)
		{
			if (col > 15) return;

			uint8_t addr = COL_TO_ADDR[col];

			_mcp.digitalWrite(PIN_A0, (addr >> 0) & 1);
			_mcp.digitalWrite(PIN_A1, (addr >> 1) & 1);
			_mcp.digitalWrite(PIN_A2, (addr >> 2) & 1);
			_mcp.digitalWrite(PIN_A3, (addr >> 3) & 1);
			_mcp.digitalWrite(PIN_EN, HIGH);
		}

		void disable()
		{
			_mcp.digitalWrite(PIN_EN, LOW);
		}
	}; // end class ADG706

	constexpr uint8_t ADG706::COL_TO_ADDR[16];

} // end namespace crt
