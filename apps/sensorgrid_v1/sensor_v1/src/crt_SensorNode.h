// by Marius Versteegen, 2025

#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <crt_SensorPacket.h>

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

		static constexpr uint8_t BROADCAST_ADDRESS[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

		static void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
		{
			if (status != ESP_NOW_SEND_SUCCESS)
			{
				ESP_LOGW("SensorNode", "Send failed");
			}
		}

	public:
		SensorNode(uint8_t sensorId, int channel, unsigned long sampleIntervalMs)
			: sensorId(sensorId), channel(channel), sampleIntervalMs(sampleIntervalMs),
			  lastSampleMs(0), counter(0)
		{
		}

		void init()
		{
			ESP_LOGI("SensorNode", "Sensor node starting, id=%u, channel=%d", sensorId, channel);

			// Turn off the onboard RGB LED (NeoPixel on GPIO 48)
			neopixelWrite(RGB_BUILTIN, 0, 0, 0);

			WiFi.mode(WIFI_STA);

			// Force WiFi channel to match the server's AP channel
			esp_wifi_set_promiscuous(true);
			esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
			esp_wifi_set_promiscuous(false);

			if (esp_now_init() != ESP_OK)
			{
				ESP_LOGE("SensorNode", "ESP-NOW init failed!");
				return;
			}
			esp_now_register_send_cb(onDataSent);

			// Add broadcast peer
			esp_now_peer_info_t peer = {};
			memcpy(peer.peer_addr, BROADCAST_ADDRESS, 6);
			peer.channel = channel;
			peer.encrypt = false;
			if (esp_now_add_peer(&peer) != ESP_OK)
			{
				ESP_LOGE("SensorNode", "Failed to add broadcast peer!");
				return;
			}

			ESP_LOGI("SensorNode", "ESP-NOW ready, STA MAC: %s", WiFi.macAddress().c_str());
		}

		void update()
		{
			unsigned long now = millis();
			if (now - lastSampleMs >= sampleIntervalMs)
			{
				lastSampleMs = now;

				counter += 10 * sensorId;
				int value = counter % 1024;

				SensorPacket pkt;
				pkt.sensorId = sensorId;
				pkt.adcValue = value;

				esp_err_t res = esp_now_send(BROADCAST_ADDRESS, (uint8_t *)&pkt, sizeof(pkt));
				if (res != ESP_OK)
				{
					ESP_LOGE("SensorNode", "Send failed, err=%d", res);
				}
				else
				{
					ESP_LOGI("SensorNode", "Send: id=%u val=%d", pkt.sensorId, value);
				}
			}
		}
	}; // end class SensorNode

	constexpr uint8_t SensorNode::BROADCAST_ADDRESS[6];

} // end namespace crt
