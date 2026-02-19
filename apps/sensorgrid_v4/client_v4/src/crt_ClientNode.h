// by Marius Versteegen, 2025

#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

namespace crt
{
	class ClientNode
	{
	private:
		const char* ssid;
		const char* pass;
		const char* serverIp;
		uint16_t serverPort;

		int testsRun;
		int testsPassed;
		int testsFailed;
		bool testsComplete;

		WiFiClient wifiClient;

		void logResult(const char* testName, bool passed, const char* detail)
		{
			testsRun++;
			if (passed)
			{
				testsPassed++;
				ESP_LOGI("ClientNode", "[PASS] %s: %s", testName, detail);
			}
			else
			{
				testsFailed++;
				ESP_LOGE("ClientNode", "[FAIL] %s: %s", testName, detail);
			}
		}

		bool httpGet(const char* path, int &httpCode, String &body)
		{
			HTTPClient http;

			if (!http.begin(wifiClient, serverIp, serverPort, path))
			{
				ESP_LOGE("ClientNode", "HTTPClient begin failed for %s", path);
				return false;
			}

			httpCode = http.GET();
			if (httpCode > 0)
			{
				body = http.getString();
			}
			else
			{
				body = http.errorToString(httpCode);
			}
			http.end();
			return (httpCode > 0);
		}

		void testDashboardPage()
		{
			const char* TEST_NAME = "GET / (dashboard)";
			int code = 0;
			String body;

			if (!httpGet("/", code, body))
			{
				char msg[64];
				snprintf(msg, sizeof(msg), "HTTP request failed: code=%d", code);
				logResult(TEST_NAME, false, msg);
				return;
			}

			if (code != 200)
			{
				char msg[64];
				snprintf(msg, sizeof(msg), "Expected HTTP 200, got %d", code);
				logResult(TEST_NAME, false, msg);
				return;
			}

			bool hasTitle = body.indexOf("<title>ESP32-S3 sensormetingen</title>") >= 0;
			bool hasHeading = body.indexOf("Sensormetingen") >= 0;
			bool hasScript = body.indexOf("<script>") >= 0;
			bool hasApiRef = body.indexOf("/api/sensors") >= 0;
			bool hasNav = body.indexOf("<nav") >= 0;

			if (hasTitle && hasHeading && hasScript && hasApiRef && hasNav)
			{
				logResult(TEST_NAME, true, "HTML OK: title, heading, script, api reference, nav bar present");
			}
			else
			{
				char msg[128];
				snprintf(msg, sizeof(msg), "Missing: %s%s%s%s%s",
					hasTitle ? "" : "title ",
					hasHeading ? "" : "heading ",
					hasScript ? "" : "script ",
					hasApiRef ? "" : "apiRef ",
					hasNav ? "" : "nav ");
				logResult(TEST_NAME, false, msg);
			}
		}

		void testApiSensorsStructure()
		{
			const char* TEST_NAME = "GET /api/sensors (structure)";
			int code = 0;
			String body;

			if (!httpGet("/api/sensors", code, body))
			{
				logResult(TEST_NAME, false, "HTTP request failed");
				return;
			}

			if (code != 200)
			{
				char msg[64];
				snprintf(msg, sizeof(msg), "Expected HTTP 200, got %d", code);
				logResult(TEST_NAME, false, msg);
				return;
			}

			bool hasNow = body.indexOf("\"now\"") >= 0;
			bool hasSensors = body.indexOf("\"sensors\"") >= 0;
			bool hasId = body.indexOf("\"id\"") >= 0;
			bool hasSeen = body.indexOf("\"seen\"") >= 0;
			bool hasValue = body.indexOf("\"value\"") >= 0;

			if (hasNow && hasSensors && hasId && hasSeen && hasValue)
			{
				logResult(TEST_NAME, true, "JSON structure OK: now, sensors[], id, seen, value fields present");
			}
			else
			{
				char msg[128];
				snprintf(msg, sizeof(msg), "Missing: %s%s%s%s%s",
					hasNow ? "" : "now ",
					hasSensors ? "" : "sensors ",
					hasId ? "" : "id ",
					hasSeen ? "" : "seen ",
					hasValue ? "" : "value ");
				logResult(TEST_NAME, false, msg);
			}
		}

		void testSensorDataPresent()
		{
			const char* TEST_NAME = "Sensor data present";
			int code = 0;
			String body;

			if (!httpGet("/api/sensors", code, body))
			{
				logResult(TEST_NAME, false, "HTTP request failed");
				return;
			}

			if (code != 200)
			{
				logResult(TEST_NAME, false, "HTTP request returned non-200");
				return;
			}

			bool hasSeenTrue = body.indexOf("\"seen\":true") >= 0;
			if (hasSeenTrue)
			{
				logResult(TEST_NAME, true, "At least one sensor has seen:true");
			}
			else
			{
				logResult(TEST_NAME, false, "No sensor with seen:true found");
			}
		}

		void testSensorValuesUpdating()
		{
			const char* TEST_NAME = "Sensor values updating";
			int code1 = 0, code2 = 0;
			String body1, body2;

			if (!httpGet("/api/sensors", code1, body1))
			{
				logResult(TEST_NAME, false, "First HTTP request failed");
				return;
			}

			delay(500);

			if (!httpGet("/api/sensors", code2, body2))
			{
				logResult(TEST_NAME, false, "Second HTTP request failed");
				return;
			}

			if (body1 != body2)
			{
				logResult(TEST_NAME, true, "Sensor data changed between two polls (500ms apart)");
			}
			else
			{
				logResult(TEST_NAME, false, "Sensor data identical between two polls");
			}
		}

		void testDownloadButton()
		{
			const char* TEST_NAME = "Download button (CSV export)";
			int code = 0;
			String body;

			if (!httpGet("/", code, body))
			{
				logResult(TEST_NAME, false, "HTTP request failed");
				return;
			}

			if (code != 200)
			{
				logResult(TEST_NAME, false, "HTTP request returned non-200");
				return;
			}

			bool hasButton = body.indexOf("id=\"downloadBtn\"") >= 0;
			bool hasDownloadLabel = body.indexOf(">Download<") >= 0;
			bool hasCsvGeneration = body.indexOf("text/csv") >= 0;
			bool hasCsvHeaders = body.indexOf("Sensor id,Meetwaarde") >= 0;
			bool hasFilename = body.indexOf("sensors.csv") >= 0;

			if (hasButton && hasDownloadLabel && hasCsvGeneration && hasCsvHeaders && hasFilename)
			{
				logResult(TEST_NAME, true,
					"Download button present, CSV generation JS verified (button, text/csv, headers, filename)");
			}
			else
			{
				char msg[128];
				snprintf(msg, sizeof(msg), "Missing: %s%s%s%s%s",
					hasButton ? "" : "button ",
					hasDownloadLabel ? "" : "label ",
					hasCsvGeneration ? "" : "csv-type ",
					hasCsvHeaders ? "" : "csv-headers ",
					hasFilename ? "" : "filename ");
				logResult(TEST_NAME, false, msg);
			}
		}

		void testGridPage()
		{
			const char* TEST_NAME = "GET /grid (grid view)";
			int code = 0;
			String body;

			if (!httpGet("/grid", code, body))
			{
				logResult(TEST_NAME, false, "HTTP request failed");
				return;
			}

			if (code != 200)
			{
				char msg[64];
				snprintf(msg, sizeof(msg), "Expected HTTP 200, got %d", code);
				logResult(TEST_NAME, false, msg);
				return;
			}

			bool hasTitle = body.indexOf("Grid View") >= 0;
			bool hasNav = body.indexOf("<nav") >= 0;
			bool hasGridContainer = body.indexOf("grid-container") >= 0;
			bool hasSensor1 = body.indexOf("Sensor 1") >= 0;
			bool hasSensor4 = body.indexOf("Sensor 4") >= 0;
			bool hasAllApi = body.indexOf("allmeasurements") >= 0;
			bool hasHistogram = body.indexOf("histogram") >= 0;
			bool hasStatsTable = body.indexOf("stats-table") >= 0;
			bool hasNormalize = body.indexOf("Normalize") >= 0;
			bool hasColorize = body.indexOf("Colorize") >= 0;
			bool hasLayout = body.indexOf("sensor-layout") >= 0;

			if (hasTitle && hasNav && hasGridContainer && hasSensor1 && hasSensor4 &&
				hasAllApi && hasHistogram && hasStatsTable && hasNormalize && hasColorize && hasLayout)
			{
				logResult(TEST_NAME, true, "Grid page OK: title, nav, grid, sensors 1-4, allapi, histogram, stats, buttons, layout");
			}
			else
			{
				char msg[180];
				snprintf(msg, sizeof(msg), "Missing: %s%s%s%s%s%s%s%s%s%s%s",
					hasTitle ? "" : "title ",
					hasNav ? "" : "nav ",
					hasGridContainer ? "" : "grid ",
					hasSensor1 ? "" : "s1 ",
					hasSensor4 ? "" : "s4 ",
					hasAllApi ? "" : "allapi ",
					hasHistogram ? "" : "histogram ",
					hasStatsTable ? "" : "stats ",
					hasNormalize ? "" : "normalize ",
					hasColorize ? "" : "colorize ",
					hasLayout ? "" : "layout ");
				logResult(TEST_NAME, false, msg);
			}
		}

		void testApiMeasurements()
		{
			const char* TEST_NAME = "GET /api/measurements/1 (sensor 1 measurements)";
			int code = 0;
			String body;

			if (!httpGet("/api/measurements/1", code, body))
			{
				logResult(TEST_NAME, false, "HTTP request failed");
				return;
			}

			if (code != 200)
			{
				char msg[64];
				snprintf(msg, sizeof(msg), "Expected HTTP 200, got %d", code);
				logResult(TEST_NAME, false, msg);
				return;
			}

			bool hasId = body.indexOf("\"id\":1") >= 0;
			bool hasCount = body.indexOf("\"count\":") >= 0;
			bool hasValues = body.indexOf("\"values\":[") >= 0;

			if (hasId && hasCount && hasValues)
			{
				logResult(TEST_NAME, true, "Measurements JSON OK: id, count, values[] present");
			}
			else
			{
				char msg[128];
				snprintf(msg, sizeof(msg), "Missing: %s%s%s",
					hasId ? "" : "id ",
					hasCount ? "" : "count ",
					hasValues ? "" : "values ");
				logResult(TEST_NAME, false, msg);
			}
		}

		void testApiAllMeasurements()
		{
			const char* TEST_NAME = "GET /api/allmeasurements (all sensors)";
			int code = 0;
			String body;

			if (!httpGet("/api/allmeasurements", code, body))
			{
				logResult(TEST_NAME, false, "HTTP request failed");
				return;
			}

			if (code != 200)
			{
				char msg[64];
				snprintf(msg, sizeof(msg), "Expected HTTP 200, got %d", code);
				logResult(TEST_NAME, false, msg);
				return;
			}

			bool hasSensors = body.indexOf("\"sensors\":[") >= 0;
			bool hasId1 = body.indexOf("\"id\":1") >= 0;
			bool hasId2 = body.indexOf("\"id\":2") >= 0;
			bool hasCount = body.indexOf("\"count\":") >= 0;
			bool hasValues = body.indexOf("\"values\":[") >= 0;

			if (hasSensors && hasId1 && hasId2 && hasCount && hasValues)
			{
				logResult(TEST_NAME, true, "All measurements JSON OK: sensors[], id:1, id:2, count, values[] present");
			}
			else
			{
				char msg[128];
				snprintf(msg, sizeof(msg), "Missing: %s%s%s%s%s",
					hasSensors ? "" : "sensors ",
					hasId1 ? "" : "id1 ",
					hasId2 ? "" : "id2 ",
					hasCount ? "" : "count ",
					hasValues ? "" : "values ");
				logResult(TEST_NAME, false, msg);
			}
		}

		void testNotFound()
		{
			const char* TEST_NAME = "GET /nonexistent (404)";
			int code = 0;
			String body;

			HTTPClient http;
			if (!http.begin(wifiClient, serverIp, serverPort, "/nonexistent"))
			{
				logResult(TEST_NAME, false, "HTTPClient begin failed");
				return;
			}

			code = http.GET();
			http.end();

			if (code == 404)
			{
				logResult(TEST_NAME, true, "Correctly returned HTTP 404");
			}
			else
			{
				char msg[64];
				snprintf(msg, sizeof(msg), "Expected HTTP 404, got %d", code);
				logResult(TEST_NAME, false, msg);
			}
		}

	public:
		ClientNode(const char* ssid, const char* pass,
				   const char* serverIp, uint16_t serverPort)
			: ssid(ssid), pass(pass), serverIp(serverIp), serverPort(serverPort),
			  testsRun(0), testsPassed(0), testsFailed(0), testsComplete(false)
		{
		}

		void init()
		{
			ESP_LOGI("ClientNode", "Client test node v4 starting...");

			neopixelWrite(RGB_BUILTIN, 0, 0, 0);

			WiFi.mode(WIFI_STA);
			WiFi.begin(ssid, pass);
			ESP_LOGI("ClientNode", "Connecting to WiFi AP '%s'...", ssid);

			int attempts = 0;
			while (WiFi.status() != WL_CONNECTED && attempts < 20)
			{
				delay(500);
				attempts++;
			}

			if (WiFi.status() == WL_CONNECTED)
			{
				ESP_LOGI("ClientNode", "WiFi connected, IP: %s", WiFi.localIP().toString().c_str());
			}
			else
			{
				ESP_LOGE("ClientNode", "WiFi connection failed after %d attempts", attempts);
			}
		}

		void update()
		{
			if (testsComplete)
			{
				return;
			}

			if (WiFi.status() != WL_CONNECTED)
			{
				ESP_LOGE("ClientNode", "WiFi not connected, cannot run tests");
				testsComplete = true;
				return;
			}

			ESP_LOGI("ClientNode", "========================================");
			ESP_LOGI("ClientNode", "Starting server_v4 web endpoint tests...");
			ESP_LOGI("ClientNode", "========================================");

			testDashboardPage();
			testGridPage();
			testApiSensorsStructure();
			testApiMeasurements();
			testApiAllMeasurements();
			testSensorDataPresent();
			testSensorValuesUpdating();
			testDownloadButton();
			testNotFound();

			ESP_LOGI("ClientNode", "========================================");
			ESP_LOGI("ClientNode", "Test summary: %d/%d passed, %d failed",
				testsPassed, testsRun, testsFailed);
			ESP_LOGI("ClientNode", "========================================");

			testsComplete = true;
		}
	}; // end class ClientNode

} // end namespace crt
