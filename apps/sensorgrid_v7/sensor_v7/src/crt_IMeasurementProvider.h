// by Marius Versteegen, 2025
// Abstract interface for measurement providers (stub or real hardware).

#pragma once
#include <cstdint>
#include <crt_SensorGridPacket.h>

namespace crt
{
	class IMeasurementProvider
	{
	public:
		virtual ~IMeasurementProvider() {}

		// Initialize the measurement hardware. Returns true if ready.
		virtual bool init() = 0;

		// Perform one measurement cycle, writing MEASUREMENT_COUNT
		// uint16_t values into the provided buffer.
		virtual void measure(uint16_t* buffer) = 0;
	};

} // end namespace crt
