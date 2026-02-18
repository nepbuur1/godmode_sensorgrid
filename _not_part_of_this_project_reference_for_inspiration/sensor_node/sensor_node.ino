#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// ====== pinconfig ======
#define WAKEUP_PIN 14   // knop, active LOW, ook voor extra "blijf wakker"
#define ADC_PIN    34   // analoge ingang
#define LED_PIN     2   // onboard LED (GPIO2)

// ====== sensor-id ======
const uint8_t SENSOR_ID = 1;    // <--- zet deze voor het branden op de juiste waarde.

// ====== ESP-NOW ontvanger MAC-adres ======
// (jij had deze in je bestand staan)
uint8_t receiverMac[] = { 0x94, 0xB5, 0x55, 0x2D, 0x00, 0xB5 };

// ====== instellingen ======
const int FIXED_CHANNEL = 1;            // zelfde kanaal als je AP/collector
const unsigned long ACTIVE_MS = 20000;  // zó lang wakker blijven zonder activiteit (20 s)
const unsigned long SAMPLE_MS = 100;    // elke 100 ms meten en sturen
const float CHANGE_THRESHOLD = 0.10f;   // 10% verandering = activiteit

// pakket dat we versturen
struct SensorPacket {
  uint8_t sensorId;
  int adcValue;
};

// callback voor verzenden (nieuwe esp32 core)
void onDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  // alleen voor debug
  Serial.print("[ESP-NOW] send status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
}

// in slaap gaan
void goToSleep() {
  // wakker worden op WAKEUP_PIN = LOW
  esp_sleep_enable_ext0_wakeup((gpio_num_t)WAKEUP_PIN, 0);
  Serial.println("➡️  ga nu in deep sleep");
  Serial.flush();
  delay(50);
  esp_deep_sleep_start();
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println("=== SENSOR NODE (met activiteit-reset) ===");

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  pinMode(WAKEUP_PIN, INPUT_PULLUP);

  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
  if (cause == ESP_SLEEP_WAKEUP_EXT0) {
    Serial.println("Wakker geworden door knop (EXT0).");
  } else {
    Serial.println("Eerste boot of andere wakeup.");
  }

  // WiFi op STA en kanaal forceren
  WiFi.mode(WIFI_STA);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(FIXED_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  // ESP-NOW init
  if (esp_now_init() != ESP_OK) {
    Serial.println("❌ ESP-NOW init mislukt, ga slapen");
    goToSleep();
  }
  esp_now_register_send_cb(onDataSent);

  // peer toevoegen
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, receiverMac, 6);
  peer.channel = FIXED_CHANNEL;
  peer.encrypt = false;
  if (esp_now_add_peer(&peer) != ESP_OK) {
    Serial.println("❌ Peer toevoegen mislukt, ga slapen");
    goToSleep();
  }

  Serial.println("✅ ESP-NOW klaar, ga meten...");
  digitalWrite(LED_PIN, HIGH);
}

void loop() {
  static unsigned long lastActivityMs = millis();   // laatst dat we iets “actiefs” deden
  static unsigned long lastSampleMs   = 0;
  static int lastValue                = -1;         // vorige ADC-waarde

  unsigned long now = millis();

  // 1) knop nogmaals ingedrukt? → timer resetten
  if (digitalRead(WAKEUP_PIN) == LOW) {
    lastActivityMs = now;
    // Serial.println("Knop tijdens wake → timer reset");
  }

  // 2) tijd voor nieuwe meting?
  if (now - lastSampleMs >= SAMPLE_MS) {
    lastSampleMs = now;

    int adc = analogRead(ADC_PIN);

    // pakket vullen
    SensorPacket pkt;
    pkt.sensorId = SENSOR_ID;
    pkt.adcValue = adc;

    // verzenden
    esp_err_t res = esp_now_send(receiverMac, (uint8_t *)&pkt, sizeof(pkt));
    if (res != ESP_OK) {
      Serial.printf("Versturen mislukt, code=%d\n", res);
    } else {
      Serial.printf("Send: id=%u val=%d\n", pkt.sensorId, pkt.adcValue);
    }

    // 3) kijken of de waarde “veel” veranderd is
    if (lastValue >= 0) {
      int diff = abs(adc - lastValue);
      int base = lastValue;
      if (base < 10) base = 10;  // beveiliging tegen heel kleine waardes
      float ratio = (float)diff / (float)base;
      if (ratio > CHANGE_THRESHOLD) {
        // meer dan 10% verschil → opnieuw de 20 seconden starten
        lastActivityMs = now;
        // Serial.println("Grote verandering → timer reset");
      }
    } else {
      // eerste meting na wake → ook als activiteit tellen
      lastActivityMs = now;
    }

    lastValue = adc;
  }

  // 4) te lang niets → slapen
  if (now - lastActivityMs >= ACTIVE_MS) {
    digitalWrite(LED_PIN, LOW);
    goToSleep();
  }

  delay(5);
}
