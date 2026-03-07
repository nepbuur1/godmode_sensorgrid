// by Marius Versteegen, 2025

#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Wire.h>
#include <crt_SensorGridPacket.h>
#include "crt_IMeasurementProvider.h"
#include "crt_StubMeasurement.h"
#include "crt_MCP23017.h"
#include "crt_RealMeasurement.h"

namespace crt
{
	class SensorNode
	{
	private:
		uint8_t sensorId;
		int channel;
		unsigned long sampleIntervalMs;
		unsigned long lastSampleMs;

		// Double-buffered measurements: update() writes to one buffer,
		// handlePoll() reads from the other. No race condition.
		uint16_t measurements[2][MEASUREMENT_COUNT];
		volatile uint8_t readyIndex;

		// Measurement provider (stub or real)
		IMeasurementProvider* measurementProvider;
		StubMeasurement stubProvider;
		RealMeasurement realProvider;

		static SensorNode* instance;
		static bool serverPeerAdded;
		static uint8_t serverMac[6];

		static void onDataRecv(const esp_now_recv_info_t* info,
							   const uint8_t* incomingData, int len)
		{
			if (len < 1 || instance == nullptr) return;
			MessageType msgType = static_cast<MessageType>(incomingData[0]);

			switch (msgType)
			{
				case MessageType::DISCOVER:
					instance->handleDiscover(info->src_addr);
					break;
				case MessageType::POLL:
					if (len >= (int)sizeof(PollPacket))
					{
						PollPacket pkt;
						memcpy(&pkt, incomingData, sizeof(pkt));
						if (pkt.sensorId == instance->sensorId)
						{
							instance->handlePoll(info->src_addr);
						}
					}
					break;
				default:
					break;
			}
		}

		static void onDataSent(const uint8_t* mac_addr, esp_now_send_status_t status)
		{
			if (status != ESP_NOW_SEND_SUCCESS)
			{
				ESP_LOGW("SensorNode", "Send failed");
			}
		}

		void ensureServerPeer(const uint8_t* mac)
		{
			if (!serverPeerAdded)
			{
				esp_now_peer_info_t peer = {};
				memcpy(peer.peer_addr, mac, 6);
				peer.channel = channel;
				peer.encrypt = false;
				if (esp_now_add_peer(&peer) == ESP_OK)
				{
					memcpy(serverMac, mac, 6);
					serverPeerAdded = true;
					ESP_LOGI("SensorNode", "Added server peer %02X:%02X:%02X:%02X:%02X:%02X",
						mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
				}
			}
		}

		void handleDiscover(const uint8_t* mac)
		{
			ESP_LOGI("SensorNode", "Received DISCOVER, sending REGISTER id=%u", sensorId);
			ensureServerPeer(mac);

			RegisterPacket reg;
			reg.messageType = MessageType::REGISTER;
			reg.sensorId = sensorId;
			esp_now_send(mac, (uint8_t*)&reg, sizeof(reg));
		}

		void handlePoll(const uint8_t* mac)
		{
			ensureServerPeer(mac);

			const uint16_t totalBytes = MEASUREMENT_COUNT * sizeof(uint16_t);
			const uint8_t maxPerPacket = DATA_PAYLOAD_MAX_SIZE;
			const uint8_t totalPackets = (totalBytes + maxPerPacket - 1) / maxPerPacket;
			const uint8_t* src = (const uint8_t*)measurements[readyIndex];

			uint16_t offset = 0;
			for (uint8_t i = 0; i < totalPackets; i++)
			{
				uint16_t remaining = totalBytes - offset;
				uint8_t chunk = (remaining > maxPerPacket) ? maxPerPacket : (uint8_t)remaining;

				DataPacket data = {};
				data.messageType = MessageType::DATA;
				data.sensorId = sensorId;
				data.packetIndex = i;
				data.totalPackets = totalPackets;
				data.payloadSize = chunk;
				memcpy(data.payload, src + offset, chunk);

				size_t sendSize = sizeof(DataPacket) - DATA_PAYLOAD_MAX_SIZE + chunk;
				esp_now_send(mac, (uint8_t*)&data, sendSize);
				offset += chunk;
			}

			ESP_LOGI("SensorNode", "Received POLL, sent %u pkt(s) id=%u val=%u (%u measurements)",
					 totalPackets, sensorId, measurements[readyIndex][0], MEASUREMENT_COUNT);
		}

	public:
		SensorNode(uint8_t sensorId, int channel, unsigned long sampleIntervalMs)
			: sensorId(sensorId), channel(channel),
			  sampleIntervalMs(sampleIntervalMs),
			  lastSampleMs(0), readyIndex(0),
			  measurementProvider(nullptr),
			  stubProvider(sensorId),
			  realProvider()
		{
			memset(measurements, 0, sizeof(measurements));
			instance = this;
		}

		void init()
		{
			ESP_LOGI("SensorNode", "Sensor node v5 starting, id=%u, channel=%d",
					 sensorId, channel);

			rgbLedWrite(RGB_BUILTIN, 0, 0, 0);

			// Reset MCP23017 via hardware reset pin before probing I2C
			::pinMode(14, OUTPUT);
			::digitalWrite(14, LOW);
			delay(10);
			::digitalWrite(14, HIGH);
			delay(10);

			// Detect MCP23017 on I2C to choose measurement mode
			if (MCP23017::detect(0x24, &Wire, 4, 5))
			{
				ESP_LOGI("SensorNode", "MCP23017 detected, using real measurement");
				measurementProvider = &realProvider;
			}
			else
			{
				ESP_LOGI("SensorNode", "MCP23017 not detected, using stub measurement");
				measurementProvider = &stubProvider;
			}
			measurementProvider->init();

			// WiFi + ESP-NOW init
			WiFi.mode(WIFI_STA);

			esp_wifi_set_promiscuous(true);
			esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
			esp_wifi_set_promiscuous(false);

			if (esp_now_init() != ESP_OK)
			{
				ESP_LOGE("SensorNode", "ESP-NOW init failed!");
				return;
			}
			esp_now_register_recv_cb(onDataRecv);
			esp_now_register_send_cb(onDataSent);

			ESP_LOGI("SensorNode", "ESP-NOW ready, STA MAC: %s",
					 WiFi.macAddress().c_str());
		}

		void update()
		{
			unsigned long now = millis();
			if (now - lastSampleMs >= sampleIntervalMs)
			{
				lastSampleMs = now;

				// Write to the non-ready buffer (safe from handlePoll reads)
				uint8_t writeIdx = 1 - readyIndex;

				// Delegate to the active measurement provider
				measurementProvider->measure(measurements[writeIdx]);

				// Atomic swap: single byte write = atomic on ESP32-S3
				readyIndex = writeIdx;
			}
		}
	}; // end class SensorNode

	SensorNode* SensorNode::instance = nullptr;
	bool SensorNode::serverPeerAdded = false;
	uint8_t SensorNode::serverMac[6] = {};

} // end namespace crt
