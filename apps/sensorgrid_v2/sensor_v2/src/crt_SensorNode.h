// by Marius Versteegen, 2025

#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <crt_SensorGridPacket.h>

namespace crt
{
	class SensorNode
	{
	private:
		uint8_t sensorId;
		int channel;
		unsigned long sampleIntervalMs;
		unsigned long lastSampleMs;
		int counter;
		int currentValue;

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

			DataPacket data = {};
			data.messageType = MessageType::DATA;
			data.sensorId = sensorId;
			data.packetIndex = 0;
			data.totalPackets = 1;
			data.payloadSize = sizeof(int);
			memcpy(data.payload, &currentValue, sizeof(int));

			size_t sendSize = sizeof(DataPacket) - DATA_PAYLOAD_MAX_SIZE + data.payloadSize;
			esp_now_send(mac, (uint8_t*)&data, sendSize);

			ESP_LOGI("SensorNode", "Received POLL, sent DATA id=%u val=%d", sensorId, currentValue);
		}

	public:
		SensorNode(uint8_t sensorId, int channel, unsigned long sampleIntervalMs)
			: sensorId(sensorId), channel(channel),
			  sampleIntervalMs(sampleIntervalMs),
			  lastSampleMs(0), counter(0), currentValue(0)
		{
			instance = this;
		}

		void init()
		{
			ESP_LOGI("SensorNode", "Sensor node v2 starting, id=%u, channel=%d",
					 sensorId, channel);

			neopixelWrite(RGB_BUILTIN, 0, 0, 0);

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
				counter += 10 * sensorId;
				currentValue = counter % 1024;
			}
		}
	}; // end class SensorNode

	SensorNode* SensorNode::instance = nullptr;
	bool SensorNode::serverPeerAdded = false;
	uint8_t SensorNode::serverMac[6] = {};

} // end namespace crt
