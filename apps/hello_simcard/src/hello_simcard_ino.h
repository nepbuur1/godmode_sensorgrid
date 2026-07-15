// by Marius Versteegen, 2026

#pragma once
#include <Arduino.h>
#include "crt_SdCard.h"
#include "crt_SdCardTest.h"

// Wiring of the microSD module, as established in phase 7a.
// The module has an AMS1117 regulator on board, so it is fed from 5V, not 3V3.
// These are the SPI2 (FSPI) IO_MUX pins.
static const gpio_num_t SD_PIN_CS   = GPIO_NUM_10;   // FSPICS0
static const gpio_num_t SD_PIN_MOSI = GPIO_NUM_11;   // FSPID
static const gpio_num_t SD_PIN_SCLK = GPIO_NUM_12;   // FSPICLK
static const gpio_num_t SD_PIN_MISO = GPIO_NUM_13;   // FSPIQ

// 20 MHz. Do not raise this: anything above 20000 makes the driver switch the card
// into SDR25 high-speed mode, and both cards tested then stop answering SEND_CSD,
// so the mount fails. Measured, not assumed - see hello_simcard.md. It is the mode
// switch that is refused, not the clock rate: requesting 20001 kHz fails too, while
// the actual bus clock stays 20 MHz either way.
static const int SD_FREQ_KHZ = SDMMC_FREQ_DEFAULT;

namespace crt
{
	SdCard sdCard(SD_PIN_CS, SD_PIN_MOSI, SD_PIN_MISO, SD_PIN_SCLK, SD_FREQ_KHZ);
	SdCardTest sdCardTest(sdCard);
}

void setup()
{
	ESP_LOGI("main", "=== HELLO SIMCARD ===");
	crt::sdCardTest.init();
}

void loop()
{
	crt::sdCardTest.update();
}
