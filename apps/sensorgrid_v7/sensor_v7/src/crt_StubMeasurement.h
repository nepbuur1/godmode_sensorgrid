// by Marius Versteegen, 2025
// Stub measurement provider: generates simulated sensor values.
// Used when no MCP23017 hardware is detected.

#pragma once
#include <Arduino.h>
#include "crt_IMeasurementProvider.h"

namespace crt
{
	class StubMeasurement : public IMeasurementProvider
	{
	private:
		uint8_t sensorId;
		uint16_t counter;

	public:
		StubMeasurement(uint8_t sensorId)
			: sensorId(sensorId), counter(0) {}

		bool init() override
		{
			ESP_LOGI("StubMeasurement", "Stub mode (no MCP23017 detected)");
			return true;
		}

		void measure(uint16_t* buffer) override
		{
			// Simulate 20ms measurement processing time
			delay(20);

			counter = (counter + 10 * sensorId) % 2501;
			// Fill in reversed order to match RealMeasurement inversion
			// Values range 0..2500
			for (uint8_t i = 0; i < MEASUREMENT_COUNT; i++)
			{
				buffer[MEASUREMENT_COUNT - 1 - i] = (uint16_t)((counter + i * 19) % 2501);
			}
		}
	}; // end class StubMeasurement

} // end namespace crt
