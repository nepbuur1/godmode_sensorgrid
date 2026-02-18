// by Marius Versteegen, 2025

#pragma once
#include <Arduino.h>
#include "crt_SensorNode.h"

// Change SENSOR_ID before flashing each sensor node:
// sensor_1 = 1, sensor_2 = 2, etc.
static const uint8_t SENSOR_ID = 1;

static const int FIXED_CHANNEL = 1;
static const unsigned long SAMPLE_INTERVAL_MS = 100;

namespace crt
{
	SensorNode sensorNode(SENSOR_ID, FIXED_CHANNEL, SAMPLE_INTERVAL_MS);
}

void setup()
{
	ESP_LOGI("main", "=== SENSOR NODE v3 ===");
	crt::sensorNode.init();
}

void loop()
{
	crt::sensorNode.update();
}
