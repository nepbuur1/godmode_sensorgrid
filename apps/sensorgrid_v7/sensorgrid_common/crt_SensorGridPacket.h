// by Marius Versteegen, 2025

#pragma once
#include <cstdint>

namespace crt
{
	enum class MessageType : uint8_t
	{
		DISCOVER = 0x01,
		REGISTER = 0x02,
		POLL     = 0x03,
		DATA     = 0x04
	};

	// Server -> broadcast. Tells sensors to register.
	struct DiscoverPacket
	{
		MessageType messageType;
	} __attribute__((packed));

	// Sensor -> server. Reply to DISCOVER.
	struct RegisterPacket
	{
		MessageType messageType;
		uint8_t sensorId;
	} __attribute__((packed));

	// Server -> sensor (unicast). Requests sensor data.
	struct PollPacket
	{
		MessageType messageType;
		uint8_t sensorId;
	} __attribute__((packed));

	// Number of uint16_t measurements per sensor sample cycle.
	static const uint8_t MEASUREMENT_COUNT = 128;

	// Sensor -> server. Response to POLL.
	// Supports multi-packet payloads via packetIndex/totalPackets.
	// 245 = ESP-NOW max frame (250) minus DataPacket header (5 bytes).
	static const uint8_t DATA_PAYLOAD_MAX_SIZE = 245;

	struct DataPacket
	{
		MessageType messageType;
		uint8_t sensorId;
		uint8_t packetIndex;
		uint8_t totalPackets;
		uint8_t payloadSize;
		uint8_t payload[DATA_PAYLOAD_MAX_SIZE];
	} __attribute__((packed));

	// Appended after measurement data in the DATA payload when a loadcell is present.
	struct LoadcellAppendix
	{
		uint8_t hasLoadcell;   // 0 = not present, 1 = present
		int32_t rawValue;      // raw HX711 reading (signed 24-bit extended to 32-bit)
	} __attribute__((packed));

	// Appended after LoadcellAppendix in the DATA payload.
	struct SnapshotAppendix
	{
		uint8_t snapshotRequested;  // 0 = no, 1 = snapshot requested via button press
	} __attribute__((packed));

} // end namespace crt
