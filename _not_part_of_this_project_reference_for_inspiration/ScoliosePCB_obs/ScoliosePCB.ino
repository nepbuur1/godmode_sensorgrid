#include <Wire.h>
#include <SPI.h>
#include <7Semi_MCP23017.h>

// ==========================================
//               PIN DEFINITIONS
// ==========================================

#define LED_PIN 2

// --- I2C Pins (MCP23017 + LIS3MDL) ---
#define I2C_SDA 4
#define I2C_SCL 5

// --- SPI Pins (ADS1220) ---
#define SPI_SCLK 12
#define SPI_MISO 13
#define SPI_MOSI 11
#define SPI_CS_HOST 10 

#define MCP_RESET_PIN 14   

// --- Device Addresses ---
#define LIS3MDL_ADDR 0x1C
#define MCP_ADDR     0x24  

// --- LIS3MDL Registers ---
#define WHO_AM_I     0x0F
#define CTRL_REG1    0x20
#define CTRL_REG2    0x21
#define CTRL_REG3    0x22
#define CTRL_REG4    0x23
#define OUT_X_L      0x28  

// --- ADS1220 Commands ---
#define ADS1220_CMD_RESET   0x06
#define ADS1220_CMD_START   0x08
#define ADS1220_CMD_RDATA   0x10
#define ADS1220_CMD_RREG    0x20
#define ADS1220_CMD_WREG    0x40

// --- CONSTANTS FOR CALCULATION ---
const float VREF_VOLTAGE_MV = 2500.0;     // External Reference 2.5V
const float ADS1220_MAX_CODE = 8388607.0; // 2^23 - 1

// ==========================================
//               GLOBALS
// ==========================================

MCP23017_7Semi mcp(MCP_ADDR);

pins adc_cs_pins[8] = {GPA2, GPA3, GPA4, GPA5, GPA6, GPA7, GPB0, GPB1};
const int NUM_ADCS = sizeof(adc_cs_pins) / sizeof(adc_cs_pins[0]);

float headingOffset = 0.0;
bool headingInitialized = false;

// ==========================================
//           HELPER FUNCTIONS
// ==========================================

float normalizeDeg(float deg) {
  while (deg > 180.0) deg -= 360.0;
  while (deg < -180.0) deg += 360.0;
  return deg;
}

// --- LIS3MDL Functions ---
void writeReg(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(LIS3MDL_ADDR);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

uint8_t readReg(uint8_t reg) {
  Wire.beginTransmission(LIS3MDL_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(LIS3MDL_ADDR, 1);
  return Wire.read();
}

void readMag(int16_t &x, int16_t &y, int16_t &z) {
  Wire.beginTransmission(LIS3MDL_ADDR);
  Wire.write(OUT_X_L | 0x80); // auto-increment
  Wire.endTransmission(false);
  Wire.requestFrom(LIS3MDL_ADDR, 6);

  x = Wire.read() | (Wire.read() << 8);
  y = Wire.read() | (Wire.read() << 8);
  z = Wire.read() | (Wire.read() << 8);
}

// --- ADS1220 Functions ---

void selectADC(int index) {
  mcp.digitalWrite(adc_cs_pins[index], LOW);
}

void deselectADC(int index) {
  mcp.digitalWrite(adc_cs_pins[index], HIGH);
}

void setupADC(int index) {
  selectADC(index);
  
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));
  
  // 1. Reset
  SPI.transfer(ADS1220_CMD_RESET);
  delayMicroseconds(50); 
  
  // 2. Configure Register 0
  // Goal: AIN1(Pos) - AIN0(Neg), Gain 1, PGA Bypass
  // Binary: 0011 0001 = 0x31
  SPI.transfer(ADS1220_CMD_WREG | (0x00 << 2)); 
  SPI.transfer(0x61); 

  // 3. Configure Register 1: 20 SPS
  SPI.transfer(ADS1220_CMD_WREG | (0x01 << 2));
  SPI.transfer(0x04); 

  // 4. Configure Register 2: Ext Ref 2.5V
  SPI.transfer(ADS1220_CMD_WREG | (0x02 << 2));  
  SPI.transfer(0x40); 

  // ==========================================
  //         DIAGNOSTIC READBACK
  // ==========================================
  
  // Read Register 0 (MUX)
  SPI.transfer(ADS1220_CMD_RREG | (0x00 << 2));
  uint8_t reg0 = SPI.transfer(0x00);

  // Read Register 2 (VRef)
  SPI.transfer(ADS1220_CMD_RREG | (0x02 << 2));
  uint8_t reg2 = SPI.transfer(0x00);
  
  SPI.endTransaction();
  deselectADC(index);

  // ==========================================
  //           SERIAL REPORT
  // ==========================================
  Serial.print("ADC "); Serial.print(index); 
  
  // Check if Reg0 matches what we wrote (0x31)
  if (reg0 == 0x31) {
    Serial.print(": ✅ MUX OK (Diff Mode)");
  } else {
    Serial.print(": ❌ MUX FAIL! Read: 0x"); Serial.print(reg0, HEX);
    Serial.print(" (Expected 0x31)");
  }

  // Check if Reg2 matches what we wrote (0x40)
  if (reg2 == 0x40) {
    Serial.println(" | VREF OK");
  } else {
    Serial.print(" | VREF FAIL! Read: 0x"); Serial.println(reg2, HEX);
  }
}

int32_t readADC(int index) {
  int32_t result = 0;
  
  selectADC(index);
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));
  
  SPI.transfer(ADS1220_CMD_RDATA);
  
  uint8_t msb = SPI.transfer(0x00);
  uint8_t mid = SPI.transfer(0x00);
  uint8_t lsb = SPI.transfer(0x00);
  
  SPI.endTransaction();
  deselectADC(index);
  
  result = ((uint32_t)msb << 16) | ((uint32_t)mid << 8) | lsb;
  
  if (result & 0x800000) {
    result |= 0xFF000000;
  }

  return result;
}

// ==========================================
//                 SETUP
// ==========================================

void setup() {
  pinMode(MCP_RESET_PIN, OUTPUT);
  digitalWrite(MCP_RESET_PIN, LOW);
  delay(10);
  digitalWrite(MCP_RESET_PIN, HIGH);
  delay(10);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.begin(115200);
  delay(1000); 

  Wire.begin(I2C_SDA, I2C_SCL);
  
  pinMode(SPI_CS_HOST, OUTPUT);
  digitalWrite(SPI_CS_HOST, HIGH);
  SPI.begin(SPI_SCLK, SPI_MISO, SPI_MOSI, SPI_CS_HOST);

  Serial.println("--- System Init ---");
  if (!mcp.begin()) {
    Serial.println("MCP23017 Fail");
    while (1);
  }

  // Set Extra Pins
  mcp.pinMode(GPB5, OUTPUT); mcp.digitalWrite(GPB5, HIGH);
  mcp.pinMode(GPB2, OUTPUT); mcp.digitalWrite(GPB2, LOW);
  mcp.pinMode(GPB3, OUTPUT); mcp.digitalWrite(GPB3, HIGH);
  mcp.pinMode(GPB4, OUTPUT); mcp.digitalWrite(GPB4, HIGH);
  
  for (int i = 0; i < NUM_ADCS; i++) {
    mcp.pinMode(adc_cs_pins[i], OUTPUT);
    mcp.digitalWrite(adc_cs_pins[i], HIGH); 
  }

  for (int i = 0; i < NUM_ADCS; i++) {
    setupADC(i);
    delay(10); 
  }

  uint8_t who = readReg(WHO_AM_I);
  if (who == 0x3D) {
    writeReg(CTRL_REG1, 0b01110000); 
    writeReg(CTRL_REG2, 0b00000000); 
    writeReg(CTRL_REG3, 0b00000000); 
    writeReg(CTRL_REG4, 0b00001100); 
    Serial.println("LIS3MDL OK");
  } else {
    Serial.println("LIS3MDL Fail");
  }
  
  Serial.println("Starting Plotter Stream in 2 seconds...");
  delay(2000);
}

// ==========================================
//                 LOOP
// ==========================================

void loop() {
  int16_t mx, my, mz;
  readMag(mx, my, mz);
  float heading = atan2((float)my, (float)mx) * 180.0 / PI;

  if (!headingInitialized) {
    headingOffset = heading;
    headingInitialized = true;
  }

  float relativeHeading = normalizeDeg(heading - headingOffset);

  Serial.print("Heading:");
  Serial.print(relativeHeading);
  Serial.print(",");

  for (int i = 0; i < NUM_ADCS; i++) {
    int32_t rawValue = readADC(i);
    float mv = ((float)rawValue / ADS1220_MAX_CODE) * VREF_VOLTAGE_MV;
    
    Serial.print("ADC");
    Serial.print(i);
    Serial.print(":");
    Serial.print(mv, 2);

    if (i < NUM_ADCS - 1) Serial.print(",");
  }
  
  Serial.println();

  static uint32_t lastBlink = 0;
  if (millis() - lastBlink > 500) {
    lastBlink = millis();
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  }

  delay(50);
}