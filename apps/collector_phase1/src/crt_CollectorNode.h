// by Marius Versteegen, 2025

#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <crt_SensorPacket.h>
#include "crt_IndexHtml.h"

namespace crt
{
	class CollectorNode
	{
	private:
		static const uint8_t MAX_SENSORS = 8;

		struct SensorState
		{
			bool seen;
			uint8_t id;
			int value;
			unsigned long lastSeenMs;
		};

		const char* apSsid;
		const char* apPass;
		int apChannel;
		WebServer server;

		static SensorState sensors[MAX_SENSORS + 1]; // index 1..MAX_SENSORS

		static void onDataRecv(const esp_now_recv_info_t *info,
							   const uint8_t *incomingData, int len)
		{
			unsigned long nowMs = millis();

			if (len == sizeof(SensorPacket))
			{
				SensorPacket pkt;
				memcpy(&pkt, incomingData, sizeof(pkt));
				uint8_t id = pkt.sensorId;
				if (id >= 1 && id <= MAX_SENSORS)
				{
					sensors[id].seen = true;
					sensors[id].id = id;
					sensors[id].value = pkt.adcValue;
					sensors[id].lastSeenMs = nowMs;
					ESP_LOGI("CollectorNode", "[ESP-NOW] sensor %u -> %d", id, pkt.adcValue);
				}
				else
				{
					ESP_LOGW("CollectorNode", "[ESP-NOW] unknown sensor id: %u", id);
				}
			}
			else
			{
				ESP_LOGW("CollectorNode", "[ESP-NOW] unexpected packet length=%d", len);
			}
		}

	public:
		CollectorNode(const char* ssid, const char* pass, int channel)
			: apSsid(ssid), apPass(pass), apChannel(channel), server(80)
		{
		}

		void init()
		{
			ESP_LOGI("CollectorNode", "Collector node starting...");

			for (int i = 1; i <= MAX_SENSORS; i++)
			{
				sensors[i].seen = false;
				sensors[i].id = i;
				sensors[i].value = 0;
				sensors[i].lastSeenMs = 0;
			}

			// WiFi AP + STA mode (AP for web clients, STA needed for ESP-NOW)
			WiFi.mode(WIFI_AP_STA);
			WiFi.softAP(apSsid, apPass, apChannel);
			ESP_LOGI("CollectorNode", "AP SSID: %s", apSsid);
			ESP_LOGI("CollectorNode", "AP IP: %s", WiFi.softAPIP().toString().c_str());

			// Web routes
			server.on("/", HTTP_GET, [this]() {
				server.send(200, "text/html", INDEX_HTML);
			});

			server.on("/api/sensors", HTTP_GET, [this]() {
				handleApiSensors();
			});

			server.onNotFound([this]() {
				server.send(404, "text/plain", "Not found");
			});

			server.begin();
			ESP_LOGI("CollectorNode", "WebServer started on port 80");

			// ESP-NOW
			if (esp_now_init() != ESP_OK)
			{
				ESP_LOGE("CollectorNode", "ESP-NOW init failed!");
			}
			else
			{
				ESP_LOGI("CollectorNode", "ESP-NOW init OK");
				esp_now_register_recv_cb(onDataRecv);
			}

			ESP_LOGI("CollectorNode", "STA MAC: %s", WiFi.macAddress().c_str());
			ESP_LOGI("CollectorNode", "AP MAC: %s", WiFi.softAPmacAddress().c_str());
		}

		void handleClient()
		{
			server.handleClient();
		}

	private:
		void handleApiSensors()
		{
			unsigned long nowMs = millis();

			String json = "{";
			json += "\"now\":" + String(nowMs) + ",";
			json += "\"sensors\":[";
			for (int i = 1; i <= MAX_SENSORS; i++)
			{
				if (i > 1) json += ",";
				SensorState &s = sensors[i];
				unsigned long age = s.seen ? (nowMs - s.lastSeenMs) : (unsigned long)0xFFFFFFFF;

				json += "{";
				json += "\"id\":" + String(i) + ",";
				json += "\"seen\":" + String(s.seen ? "true" : "false") + ",";
				json += "\"value\":" + String(s.seen ? s.value : 0) + ",";
				json += "\"age_ms\":" + String(s.seen ? age : (unsigned long)0xFFFFFFFF);
				json += "}";
			}
			json += "]}";

			server.send(200, "application/json", json);
		}
	}; // end class CollectorNode

	CollectorNode::SensorState CollectorNode::sensors[CollectorNode::MAX_SENSORS + 1] = {};

} // end namespace crt
