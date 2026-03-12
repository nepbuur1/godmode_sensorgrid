#include <HX711.h>
#include <Preferences.h>
#include <math.h>

// =======================
// Pin-configuratie
// =======================
const int HX_POWER_PIN = 6;   // Voeding HX711 via GPIO
const int HX_GND_PIN   = 9;   // GND HX711 via GPIO
const int HX_SCK_PIN   = 7;   // HX711 SCK
const int HX_DOUT_PIN  = 8;   // HX711 DT / DOUT

// =======================
// Objecten
// =======================
HX711 scale;
Preferences prefs;

// =======================
// Instellingen
// =======================
float calibrationFactor = 1.0f;
bool hasCalibration = false;
bool monitorMode = false;
unsigned long lastMonitorPrint = 0;
const unsigned long monitorIntervalMs = 500;

// =======================
// Helpers
// =======================
void printHelp();
void handleCommand(String cmd);
String readCommandLine();
String waitForLine(bool allowEmpty = false);
void waitForEnter();
bool hxReady(uint32_t timeoutMs = 1500);
void tareScale();
void printRaw();
void printWeight();
void startCalibration(float knownWeightGrams);
void saveCalibration(float factor);
void loadCalibration();
void printStatus();

void setup() {
  // Zet deze pinnen ZO SNEL MOGELIJK goed
  pinMode(HX_POWER_PIN, OUTPUT);
  digitalWrite(HX_POWER_PIN, HIGH);   // voed HX711

  pinMode(HX_GND_PIN, OUTPUT);
  digitalWrite(HX_GND_PIN, LOW);      // GND voor HX711

  Serial.begin(115200);
  delay(200);

  Serial.println();
  Serial.println("ESP32-S3 + HX711 weegschaal");
  Serial.println("--------------------------------");

  // Geef HX711 even tijd nadat hij via GPIO gevoed wordt
  delay(150);

  // Start HX711
  scale.begin(HX_DOUT_PIN, HX_SCK_PIN);

  // Kalibratiefactor laden uit NVS
  loadCalibration();
  scale.set_scale(calibrationFactor);

  if (hxReady()) {
    Serial.println("HX711 is gevonden.");
  } else {
    Serial.println("WAARSCHUWING: HX711 reageert niet.");
    Serial.println("Controleer bedrading en voeding.");
  }

  printStatus();
  printHelp();
}

void loop() {
  String cmd = readCommandLine();
  if (cmd.length() > 0) {
    handleCommand(cmd);
  }

  if (monitorMode && millis() - lastMonitorPrint >= monitorIntervalMs) {
    lastMonitorPrint = millis();
    printWeight();
  }
}

// =======================
// Seriële invoer
// =======================
String readCommandLine() {
  static String buffer = "";

  while (Serial.available()) {
    char c = (char)Serial.read();

    if (c == '\r') continue;

    if (c == '\n') {
      String line = buffer;
      buffer = "";
      line.trim();
      return line;
    } else {
      buffer += c;
    }
  }

  return "";
}

String waitForLine(bool allowEmpty) {
  String buffer = "";

  while (true) {
    while (Serial.available()) {
      char c = (char)Serial.read();

      if (c == '\r') continue;

      if (c == '\n') {
        String line = buffer;
        line.trim();
        if (allowEmpty || line.length() > 0) {
          return line;
        }
        buffer = "";
      } else {
        buffer += c;
      }
    }
    delay(5);
  }
}

void waitForEnter() {
  waitForLine(true);
}

// =======================
// HX711 functies
// =======================
bool hxReady(uint32_t timeoutMs) {
  return scale.wait_ready_timeout(timeoutMs);
}

void tareScale() {
  if (!hxReady()) {
    Serial.println("HX711 niet klaar.");
    return;
  }

  Serial.println("Tare bezig... Zorg dat de weegschaal leeg is.");
  scale.tare(15);
  Serial.println("Tare klaar. Weegschaal staat nu op 0.");
}

void printRaw() {
  if (!hxReady()) {
    Serial.println("HX711 niet klaar.");
    return;
  }

  long raw = scale.read_average(10);
  Serial.print("RAW gemiddelde: ");
  Serial.println(raw);
}

void printWeight() {
  if (!hxReady()) {
    Serial.println("HX711 niet klaar.");
    return;
  }

  float w = scale.get_units(10);
  Serial.print("Gewicht: ");
  Serial.print(w, 2);
  Serial.println(" g");
}

void saveCalibration(float factor) {
  prefs.begin("scale", false);
  prefs.putFloat("calFactor", factor);
  prefs.end();

  calibrationFactor = factor;
  hasCalibration = true;
}

void loadCalibration() {
  prefs.begin("scale", true);
  hasCalibration = prefs.isKey("calFactor");

  if (hasCalibration) {
    calibrationFactor = prefs.getFloat("calFactor", 1.0f);
    Serial.print("Opgeslagen kalibratiefactor geladen: ");
    Serial.println(calibrationFactor, 6);
  } else {
    calibrationFactor = 1.0f;
    Serial.println("Nog geen opgeslagen kalibratiefactor gevonden.");
    Serial.println("Gebruik 'cal 500' of een ander bekend gewicht in gram.");
  }

  prefs.end();
}

void startCalibration(float knownWeightGrams) {
  monitorMode = false;

  if (knownWeightGrams <= 0.0f) {
    Serial.println("Bekend gewicht moet groter zijn dan 0 gram.");
    return;
  }

  if (!hxReady()) {
    Serial.println("HX711 niet klaar, kalibratie afgebroken.");
    return;
  }

  Serial.println();
  Serial.println("=== KALIBRATIE START ===");
  Serial.print("Bekend gewicht: ");
  Serial.print(knownWeightGrams, 2);
  Serial.println(" g");
  Serial.println();
  Serial.println("1) Haal ALLES van de loadcell af.");
  Serial.println("2) Druk op ENTER om te taren.");
  waitForEnter();

  scale.set_scale();   // schaalfactor = 1
  scale.tare(15);

  Serial.println("Tare voltooid.");
  Serial.println();
  Serial.print("3) Plaats nu ");
  Serial.print(knownWeightGrams, 2);
  Serial.println(" g op de loadcell.");
  Serial.println("4) Wacht tot het stabiel is en druk dan ENTER.");
  waitForEnter();

  if (!hxReady()) {
    Serial.println("HX711 niet klaar tijdens kalibratie.");
    return;
  }

  float measured = scale.get_units(15);   // bij scale=1 feitelijk ruwe delta
  float newFactor = measured / knownWeightGrams;

  if (fabs(newFactor) < 0.000001f || isnan(newFactor) || isinf(newFactor)) {
    Serial.println("Ongeldige kalibratiefactor berekend.");
    Serial.println("Controleer bedrading en probeer opnieuw.");
    return;
  }

  scale.set_scale(newFactor);
  saveCalibration(newFactor);

  Serial.println();
  Serial.print("Nieuwe kalibratiefactor: ");
  Serial.println(newFactor, 6);

  Serial.print("Testmeting met geplaatst gewicht: ");
  Serial.print(scale.get_units(15), 2);
  Serial.println(" g");

  Serial.println();
  Serial.println("Kalibratie opgeslagen in ESP32 NVS.");
  Serial.println("Na een reboot blijft de factor bewaard.");
  Serial.println("Doe na opstarten wel opnieuw 'tare' als de weegschaal leeg is.");
  Serial.println("=== KALIBRATIE KLAAR ===");
}

// =======================
// Commando's
// =======================
void handleCommand(String cmd) {
  cmd.trim();
  cmd.toLowerCase();

  if (cmd == "h" || cmd == "help") {
    printHelp();
    return;
  }

  if (cmd == "tare" || cmd == "t" || cmd == "z") {
    tareScale();
    return;
  }

  if (cmd == "raw" || cmd == "r") {
    printRaw();
    return;
  }

  if (cmd == "weight" || cmd == "w") {
    printWeight();
    return;
  }

  if (cmd == "monitor" || cmd == "m") {
    monitorMode = !monitorMode;
    Serial.print("Monitor mode: ");
    Serial.println(monitorMode ? "AAN" : "UIT");
    return;
  }

  if (cmd == "factor" || cmd == "f") {
    Serial.print("Kalibratiefactor: ");
    Serial.println(calibrationFactor, 6);
    Serial.print("Kalibratie aanwezig: ");
    Serial.println(hasCalibration ? "ja" : "nee");
    return;
  }

  if (cmd == "status" || cmd == "s") {
    printStatus();
    return;
  }

  if (cmd == "clearcal") {
    prefs.begin("scale", false);
    prefs.remove("calFactor");
    prefs.end();

    calibrationFactor = 1.0f;
    hasCalibration = false;
    scale.set_scale(calibrationFactor);

    Serial.println("Opgeslagen kalibratiefactor verwijderd.");
    return;
  }

  if (cmd == "cal" || cmd == "c") {
    Serial.println("Typ het bekende gewicht in gram, bv: 500");
    String line = waitForLine(false);
    float grams = line.toFloat();
    startCalibration(grams);
    return;
  }

  if (cmd.startsWith("cal ") || cmd.startsWith("c ")) {
    int sp = cmd.indexOf(' ');
    String num = cmd.substring(sp + 1);
    float grams = num.toFloat();
    startCalibration(grams);
    return;
  }

  Serial.print("Onbekend commando: ");
  Serial.println(cmd);
  Serial.println("Typ 'help' voor de beschikbare commando's.");
}

void printStatus() {
  Serial.println();
  Serial.println("Status:");
  Serial.print("- HX711 ready: ");
  Serial.println(hxReady() ? "ja" : "nee");

  Serial.print("- Kalibratiefactor: ");
  Serial.println(calibrationFactor, 6);

  Serial.print("- Opgeslagen kalibratie: ");
  Serial.println(hasCalibration ? "ja" : "nee");

  Serial.println();
}

void printHelp() {
  Serial.println();
  Serial.println("Beschikbare commando's:");
  Serial.println("  help       - toon deze hulp");
  Serial.println("  status     - toon status");
  Serial.println("  tare       - zet huidige lege waarde op 0");
  Serial.println("  raw        - toon ruwe HX711 waarde");
  Serial.println("  weight     - toon gewicht in gram");
  Serial.println("  monitor    - gewicht continu tonen aan/uit");
  Serial.println("  factor     - toon huidige kalibratiefactor");
  Serial.println("  cal 500    - kalibreer met bekend gewicht, bv 500 gram");
  Serial.println("  clearcal   - wis opgeslagen kalibratiefactor");
  Serial.println();
  Serial.println("Tip: zet de Serial Monitor op 115200 baud");
  Serial.println("en line ending op 'Newline' of 'Both NL & CR'.");
  Serial.println();
}