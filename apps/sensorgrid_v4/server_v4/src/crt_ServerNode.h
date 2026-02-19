// by Marius Versteegen, 2025

#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <crt_SensorGridPacket.h>
#include "crt_IndexHtml.h"
#include "crt_GridHtml.h"

namespace crt
{
	class ServerNode
	{
	private:
		static const uint8_t MAX_SENSORS = 8;
		static const uint8_t MAX_POLL_RETRIES = 5;
		static const unsigned long DISCOVER_INTERVAL_MS = 500;
		static const unsigned long DATA_TIMEOUT_MS = 200;
		static const unsigned long LED_FLASH_INTERVAL_MS = 500;

		enum class State : uint8_t
		{
			DISCOVERING,
			POLLING,
			WAITING_DATA,
		};

		struct SensorState
		{
			bool registered;
			bool seen;
			uint8_t id;
			uint8_t mac[6];
			bool peerAdded;
			uint16_t measurements[MEASUREMENT_COUNT];
			uint8_t measurementCount;
			unsigned long lastSeenMs;
		};

		const char* apSsid;
		const char* apPass;
		int apChannel;
		uint8_t expectedSensorCount;
		WebServer server;

		State currentState;
		uint8_t currentPollIndex;
		uint8_t pollRetryCount;
		unsigned long stateEnteredMs;
		unsigned long lastDiscoverMs;
		unsigned long lastLedToggleMs;
		bool ledOn;

		uint8_t registeredIds[MAX_SENSORS];
		uint8_t registeredCount;

		SensorState sensors[MAX_SENSORS + 1]; // indexed 1..MAX_SENSORS

		// Static callback data (set by ESP-NOW callbacks, read by update())
		static volatile bool newRegisterReceived;
		static volatile uint8_t receivedRegisterSensorId;
		static volatile uint8_t receivedRegisterMac[6];

		static volatile bool newDataReceived;

		// Multi-packet reassembly state
		static const uint16_t MAX_REASSEMBLY_SIZE = 500;
		static uint8_t reassemblyBuffer[MAX_REASSEMBLY_SIZE];
		static volatile uint8_t dataPacketsReceived;
		static volatile uint8_t dataPacketsExpected;
		static volatile uint16_t reassemblyBytesReceived;
		static volatile uint8_t reassemblySensorId;

		static constexpr uint8_t BROADCAST_ADDRESS[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

		// --- ESP-NOW callbacks ---

		static void onDataRecv(const esp_now_recv_info_t* info,
							   const uint8_t* incomingData, int len)
		{
			if (len < 1) return;
			MessageType msgType = static_cast<MessageType>(incomingData[0]);

			switch (msgType)
			{
				case MessageType::REGISTER:
				{
					if (len >= (int)sizeof(RegisterPacket))
					{
						RegisterPacket pkt;
						memcpy(&pkt, incomingData, sizeof(pkt));
						receivedRegisterSensorId = pkt.sensorId;
						memcpy((void*)receivedRegisterMac, info->src_addr, 6);
						newRegisterReceived = true;
						ESP_LOGI("ServerNode", "[ESP-NOW] REGISTER from sensor %u", pkt.sensorId);
					}
					break;
				}
				case MessageType::DATA:
				{
					if (len >= 5)
					{
						DataPacket pkt;
						size_t copyLen = len < (int)sizeof(DataPacket) ? len : sizeof(DataPacket);
						memcpy(&pkt, incomingData, copyLen);

						if (pkt.packetIndex == 0)
						{
							// Start new reassembly
							reassemblySensorId = pkt.sensorId;
							dataPacketsExpected = pkt.totalPackets;
							dataPacketsReceived = 0;
							reassemblyBytesReceived = 0;
						}

						if (pkt.sensorId == reassemblySensorId &&
							pkt.packetIndex == dataPacketsReceived)
						{
							uint16_t offset = reassemblyBytesReceived;
							if (offset + pkt.payloadSize <= MAX_REASSEMBLY_SIZE)
							{
								memcpy(reassemblyBuffer + offset, pkt.payload, pkt.payloadSize);
								reassemblyBytesReceived += pkt.payloadSize;
							}
							dataPacketsReceived++;

							ESP_LOGI("ServerNode", "[ESP-NOW] DATA from sensor %u, pkt %u/%u (%u bytes)",
									 pkt.sensorId, pkt.packetIndex + 1, pkt.totalPackets, pkt.payloadSize);

							if (dataPacketsReceived >= dataPacketsExpected)
							{
								newDataReceived = true;
							}
						}
					}
					break;
				}
				default:
					break;
			}
		}

		static void onDataSent(const uint8_t* mac_addr, esp_now_send_status_t status)
		{
			if (status != ESP_NOW_SEND_SUCCESS)
			{
				ESP_LOGW("ServerNode", "Send failed");
			}
		}

		// --- Helper methods ---

		void ensureSensorPeer(uint8_t sensorId)
		{
			SensorState& s = sensors[sensorId];
			if (!s.peerAdded && s.registered)
			{
				esp_now_peer_info_t peer = {};
				memcpy(peer.peer_addr, s.mac, 6);
				peer.channel = apChannel;
				peer.encrypt = false;
				esp_now_add_peer(&peer);
				s.peerAdded = true;
			}
		}

		void broadcastDiscover()
		{
			DiscoverPacket disc;
			disc.messageType = MessageType::DISCOVER;
			esp_now_send(BROADCAST_ADDRESS, (uint8_t*)&disc, sizeof(disc));
			ESP_LOGI("ServerNode", "Broadcast DISCOVER (%u/%u registered)",
					 registeredCount, expectedSensorCount);
		}

		void processRegister()
		{
			if (!newRegisterReceived) return;
			newRegisterReceived = false;

			uint8_t id = receivedRegisterSensorId;
			if (id < 1 || id > MAX_SENSORS) return;

			if (!sensors[id].registered)
			{
				sensors[id].registered = true;
				sensors[id].id = id;
				memcpy(sensors[id].mac, (const void*)receivedRegisterMac, 6);
				sensors[id].peerAdded = false;

				registeredIds[registeredCount] = id;
				registeredCount++;

				ESP_LOGI("ServerNode", "Registered sensor %u (%u/%u) MAC=%02X:%02X:%02X:%02X:%02X:%02X",
						 id, registeredCount, expectedSensorCount,
						 sensors[id].mac[0], sensors[id].mac[1], sensors[id].mac[2],
						 sensors[id].mac[3], sensors[id].mac[4], sensors[id].mac[5]);
			}
		}

		bool anySensorMissing()
		{
			if (registeredCount < expectedSensorCount) return true;
			for (uint8_t i = 0; i < registeredCount; i++)
			{
				if (!sensors[registeredIds[i]].registered) return true;
			}
			return false;
		}

		void updateLed()
		{
			if (anySensorMissing())
			{
				unsigned long now = millis();
				if (now - lastLedToggleMs >= LED_FLASH_INTERVAL_MS)
				{
					lastLedToggleMs = now;
					ledOn = !ledOn;
					neopixelWrite(RGB_BUILTIN, ledOn ? 20 : 0, 0, 0);
				}
			}
			else
			{
				if (ledOn)
				{
					neopixelWrite(RGB_BUILTIN, 0, 0, 0);
					ledOn = false;
				}
			}
		}

		// --- State handlers ---

		void handleDiscovering()
		{
			processRegister();

			unsigned long now = millis();
			if (now - lastDiscoverMs >= DISCOVER_INTERVAL_MS)
			{
				lastDiscoverMs = now;
				broadcastDiscover();
			}

			if (registeredCount >= expectedSensorCount)
			{
				ESP_LOGI("ServerNode", "All %u sensors registered, starting POLL cycle",
						 expectedSensorCount);
				currentPollIndex = 0;
				currentState = State::POLLING;
			}
		}

		void handlePolling()
		{
			processRegister();

			// Cycle complete?
			if (currentPollIndex >= registeredCount)
			{
				if (anySensorMissing())
				{
					broadcastDiscover();
				}
				currentPollIndex = 0;
			}

			// Skip unregistered sensors
			while (currentPollIndex < registeredCount &&
				   !sensors[registeredIds[currentPollIndex]].registered)
			{
				currentPollIndex++;
			}

			if (currentPollIndex >= registeredCount)
			{
				// All unregistered, broadcast and reset
				broadcastDiscover();
				currentPollIndex = 0;
				return;
			}

			uint8_t sensorId = registeredIds[currentPollIndex];
			ensureSensorPeer(sensorId);

			PollPacket poll;
			poll.messageType = MessageType::POLL;
			poll.sensorId = sensorId;

			newDataReceived = false;
			esp_now_send(sensors[sensorId].mac, (uint8_t*)&poll, sizeof(poll));

			pollRetryCount = 0;
			stateEnteredMs = millis();
			currentState = State::WAITING_DATA;
		}

		void handleWaitingData()
		{
			processRegister();

			if (newDataReceived)
			{
				newDataReceived = false;
				uint8_t expectedId = registeredIds[currentPollIndex];

				if (reassemblySensorId == expectedId)
				{
					uint8_t count = reassemblyBytesReceived / sizeof(uint16_t);
					if (count > MEASUREMENT_COUNT) count = MEASUREMENT_COUNT;

					memcpy(sensors[expectedId].measurements, reassemblyBuffer,
						   count * sizeof(uint16_t));
					sensors[expectedId].measurementCount = count;
					sensors[expectedId].lastSeenMs = millis();
					sensors[expectedId].seen = true;

					ESP_LOGI("ServerNode", "Sensor %u -> %u measurements, first=%u",
							 expectedId, count, sensors[expectedId].measurements[0]);

					currentPollIndex++;
					currentState = State::POLLING;
				}
			}
			else if (millis() - stateEnteredMs >= DATA_TIMEOUT_MS)
			{
				pollRetryCount++;
				uint8_t expectedId = registeredIds[currentPollIndex];

				if (pollRetryCount > MAX_POLL_RETRIES)
				{
					ESP_LOGW("ServerNode",
							 "Sensor %u unresponsive after %u retries, marking unregistered",
							 expectedId, MAX_POLL_RETRIES);
					sensors[expectedId].registered = false;
					sensors[expectedId].peerAdded = false;

					// Remove peer so it can be re-added after re-registration
					esp_now_del_peer(sensors[expectedId].mac);

					// Remove from registeredIds by shifting
					for (uint8_t i = currentPollIndex; i < registeredCount - 1; i++)
					{
						registeredIds[i] = registeredIds[i + 1];
					}
					registeredCount--;

					currentState = State::POLLING;
				}
				else
				{
					ESP_LOGW("ServerNode", "Sensor %u timeout, retry %u/%u",
							 expectedId, pollRetryCount, MAX_POLL_RETRIES);

					PollPacket poll;
					poll.messageType = MessageType::POLL;
					poll.sensorId = expectedId;
					esp_now_send(sensors[expectedId].mac, (uint8_t*)&poll, sizeof(poll));
					stateEnteredMs = millis();
				}
			}
		}

		// --- Web server ---

		void handleApiSensors()
		{
			unsigned long nowMs = millis();

			String json = "{";
			json += "\"now\":" + String(nowMs) + ",";
			json += "\"sensors\":[";
			for (int i = 1; i <= MAX_SENSORS; i++)
			{
				if (i > 1) json += ",";
				SensorState& s = sensors[i];
				unsigned long age = s.seen ? (nowMs - s.lastSeenMs) : (unsigned long)0xFFFFFFFF;

				json += "{";
				json += "\"id\":" + String(i) + ",";
				json += "\"seen\":" + String(s.seen ? "true" : "false") + ",";
				json += "\"value\":" + String(s.seen ? (int)s.measurements[0] : 0) + ",";
				json += "\"age_ms\":" + String(s.seen ? age : (unsigned long)0xFFFFFFFF);
				json += "}";
			}
			json += "]}";

			server.send(200, "application/json", json);
		}

		void handleApiMeasurements(uint8_t sensorId)
		{
			if (sensorId < 1 || sensorId > MAX_SENSORS || !sensors[sensorId].seen)
			{
				server.send(404, "application/json", "{\"error\":\"sensor not found\"}");
				return;
			}

			SensorState& s = sensors[sensorId];
			String json = "{";
			json += "\"id\":" + String(sensorId) + ",";
			json += "\"count\":" + String(s.measurementCount) + ",";
			json += "\"values\":[";
			for (uint8_t i = 0; i < s.measurementCount; i++)
			{
				if (i > 0) json += ",";
				json += String(s.measurements[i]);
			}
			json += "]}";

			server.send(200, "application/json", json);
		}

	public:
		ServerNode(const char* ssid, const char* pass, int channel,
				   uint8_t expectedSensors)
			: apSsid(ssid), apPass(pass), apChannel(channel),
			  expectedSensorCount(expectedSensors), server(80),
			  currentState(State::DISCOVERING), currentPollIndex(0),
			  pollRetryCount(0), stateEnteredMs(0), lastDiscoverMs(0),
			  lastLedToggleMs(0), ledOn(false), registeredCount(0)
		{
		}

		void init()
		{
			ESP_LOGI("ServerNode", "Server node v4 starting...");

			neopixelWrite(RGB_BUILTIN, 0, 0, 0);

			for (int i = 1; i <= MAX_SENSORS; i++)
			{
				sensors[i] = {};
			}

			WiFi.mode(WIFI_AP_STA);
			WiFi.softAP(apSsid, apPass, apChannel);
			ESP_LOGI("ServerNode", "AP SSID: %s", apSsid);
			ESP_LOGI("ServerNode", "AP IP: %s", WiFi.softAPIP().toString().c_str());

			server.on("/", HTTP_GET, [this]() {
				server.send(200, "text/html", INDEX_HTML);
			});
			server.on("/grid", HTTP_GET, [this]() {
				server.send(200, "text/html", GRID_HTML);
			});
			server.on("/api/sensors", HTTP_GET, [this]() {
				handleApiSensors();
			});
			server.on("/api/measurements/1", HTTP_GET, [this]() {
				handleApiMeasurements(1);
			});
			server.on("/api/measurements/2", HTTP_GET, [this]() {
				handleApiMeasurements(2);
			});
			server.on("/api/measurements/3", HTTP_GET, [this]() {
				handleApiMeasurements(3);
			});
			server.on("/api/measurements/4", HTTP_GET, [this]() {
				handleApiMeasurements(4);
			});
			server.onNotFound([this]() {
				server.send(404, "text/plain", "Not found");
			});
			server.begin();
			ESP_LOGI("ServerNode", "WebServer started on port 80");

			if (esp_now_init() != ESP_OK)
			{
				ESP_LOGE("ServerNode", "ESP-NOW init failed!");
			}
			else
			{
				ESP_LOGI("ServerNode", "ESP-NOW init OK");
				esp_now_register_recv_cb(onDataRecv);
				esp_now_register_send_cb(onDataSent);

				// Add broadcast peer for DISCOVER
				esp_now_peer_info_t peer = {};
				memcpy(peer.peer_addr, BROADCAST_ADDRESS, 6);
				peer.channel = apChannel;
				peer.encrypt = false;
				esp_now_add_peer(&peer);
			}

			ESP_LOGI("ServerNode", "STA MAC: %s", WiFi.macAddress().c_str());
			ESP_LOGI("ServerNode", "AP MAC: %s", WiFi.softAPmacAddress().c_str());
			ESP_LOGI("ServerNode", "Expecting %u sensors", expectedSensorCount);
		}

		void update()
		{
			server.handleClient();
			updateLed();

			switch (currentState)
			{
				case State::DISCOVERING:
					handleDiscovering();
					break;
				case State::POLLING:
					handlePolling();
					break;
				case State::WAITING_DATA:
					handleWaitingData();
					break;
			}
		}
	}; // end class ServerNode

	volatile bool ServerNode::newRegisterReceived = false;
	volatile uint8_t ServerNode::receivedRegisterSensorId = 0;
	volatile uint8_t ServerNode::receivedRegisterMac[6] = {};
	volatile bool ServerNode::newDataReceived = false;
	uint8_t ServerNode::reassemblyBuffer[ServerNode::MAX_REASSEMBLY_SIZE] = {};
	volatile uint8_t ServerNode::dataPacketsReceived = 0;
	volatile uint8_t ServerNode::dataPacketsExpected = 0;
	volatile uint16_t ServerNode::reassemblyBytesReceived = 0;
	volatile uint8_t ServerNode::reassemblySensorId = 0;
	constexpr uint8_t ServerNode::BROADCAST_ADDRESS[6];

} // end namespace crt
