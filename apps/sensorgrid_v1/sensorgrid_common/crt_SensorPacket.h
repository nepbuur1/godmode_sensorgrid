// by Marius Versteegen, 2025

#pragma once
#include <cstdint>

namespace crt
{
	// Packet sent from sensor nodes to the server via ESP-NOW.
	// Must match exactly on both sides.
	struct SensorPacket
	{
		uint8_t sensorId;
		int adcValue;
	};
} // end namespace crt
