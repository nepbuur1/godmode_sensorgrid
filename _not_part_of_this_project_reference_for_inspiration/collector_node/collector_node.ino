#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <esp_now.h>
#include <esp_wifi.h>

const char *AP_SSID   = "SCOLIOSE";
const char *AP_PASS   = "scoliose";
const int  AP_CHANNEL = 1;

WebServer server(80);

static const uint8_t MAX_SENSORS = 8;

// data die binnenkomt van de sensornodes
struct SensorPacket {
  uint8_t sensorId;
  int adcValue;
};

// toestand per sensor
struct SensorState {
  bool seen;
  uint8_t id;
  int value;
  unsigned long lastSeenMs;
};

SensorState sensors[MAX_SENSORS + 1]; // index 1..8

// ---------- helpers voor files ----------
String getContentType(const String &path) {
  if (path.endsWith(".htm") || path.endsWith(".html")) return "text/html";
  if (path.endsWith(".css")) return "text/css";
  if (path.endsWith(".js"))  return "application/javascript";
  if (path.endsWith(".json"))return "application/json";
  if (path.endsWith(".png")) return "image/png";
  if (path.endsWith(".svg")) return "image/svg+xml";
  return "text/plain";
}

bool handleFileRead(String path) {
  if (path.endsWith("/")) path += "index.html";
  if (!LittleFS.exists(path)) return false;
  File file = LittleFS.open(path, "r");
  if (!file) return false;
  String ct = getContentType(path);
  server.streamFile(file, ct);
  file.close();
  return true;
}

// ---------- ESP-NOW callback ----------
void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  unsigned long nowMs = millis();

  // we accepteren 2 formaten:
  // 1) nieuw: struct SensorPacket
  // 2) oud: alleen een int
  if (len == sizeof(SensorPacket)) {
    SensorPacket pkt;
    memcpy(&pkt, incomingData, sizeof(pkt));
    uint8_t id = pkt.sensorId;
    if (id >= 1 && id <= MAX_SENSORS) {
      sensors[id].seen = true;
      sensors[id].id = id;
      sensors[id].value = pkt.adcValue;
      sensors[id].lastSeenMs = nowMs;

      Serial.printf("[ESP-NOW] sensor %u -> %d\n", id, pkt.adcValue);
    } else {
      Serial.printf("[ESP-NOW] onbekende sensor-id: %u\n", id);
    }

  } else if (len == sizeof(int)) {
    // backward compat: als er alleen een int binnenkomt → plak het op sensor 1
    int waarde;
    memcpy(&waarde, incomingData, sizeof(waarde));
    sensors[1].seen = true;
    sensors[1].id = 1;
    sensors[1].value = waarde;
    sensors[1].lastSeenMs = nowMs;
    Serial.printf("[ESP-NOW] (oude node) sensor 1 -> %d\n", waarde);
  } else {
    Serial.printf("[ESP-NOW] vreemd pakket, lengte=%d\n", len);
  }
}

// ---------- /api/sensors ----------
void handleApiSensors() {
  unsigned long nowMs = millis();

  // we bouwen met de hand JSON
  String json = "{";
  json += "\"now\":" + String(nowMs) + ",";
  json += "\"sensors\":[";
  for (int i = 1; i <= MAX_SENSORS; i++) {
    if (i > 1) json += ",";
    SensorState &s = sensors[i];
    unsigned long age = s.seen ? (nowMs - s.lastSeenMs) : (unsigned long)0xFFFFFFFF;

    json += "{";
    json += "\"id\":" + String(i) + ",";
    json += "\"seen\":" + String(s.seen ? "true" : "false") + ",";
    json += "\"value\":" + String(s.seen ? s.value : 0) + ",";
    json += "\"age_ms\":" + String(s.seen ? age : 0xFFFFFFFF);
    json += "}";
  }
  json += "]}";

  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\nCollector met UI + /api/sensors + CSV...");

  // init sensor array
  for (int i = 1; i <= MAX_SENSORS; i++) {
    sensors[i].seen = false;
    sensors[i].id = i;
    sensors[i].value = 0;
    sensors[i].lastSeenMs = 0;
  }

  if (!LittleFS.begin()) {
    Serial.println("❌ LittleFS mount FAIL");
  } else {
    Serial.println("✅ LittleFS gemount");
  }

  // WiFi AP + STA
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID, AP_PASS, AP_CHANNEL);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  // web routes
  server.on("/", HTTP_GET, []() {
    if (!handleFileRead("/index.html")) {
      server.send(404, "text/plain", "index.html not found");
    }
  });

  server.on("/api/sensors", HTTP_GET, handleApiSensors);

  server.onNotFound([]() {
    String path = server.uri();
    if (!handleFileRead(path)) {
      server.send(404, "text/plain", "File not found");
    }
  });

  server.begin();
  Serial.println("🌐 Webserver gestart");

  // ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("❌ ESP-NOW init mislukt!");
  } else {
    Serial.println("✅ ESP-NOW init OK");
    esp_now_register_recv_cb(onDataRecv);
  }

  Serial.print("STA MAC: ");
  Serial.println(WiFi.macAddress());
  Serial.print("AP MAC: ");
  Serial.println(WiFi.softAPmacAddress());
}

void loop() {
  server.handleClient();
}
