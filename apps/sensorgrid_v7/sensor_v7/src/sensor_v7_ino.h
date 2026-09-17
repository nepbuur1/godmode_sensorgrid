// by Marius Versteegen, 2025

#pragma once
#include <Arduino.h>
#include "crt_SensorNode.h"

// Sensor id per module: sensor_1 = 1, sensor_2 = 2, etc. Wordt gezet door build_sensor.bat
// <actie> <poort> <id> (idf.py -D SENSORGRID_SENSOR_ID=<id>); zonder die variabele geldt de
// waarde hieronder.
#ifndef SENSORGRID_SENSOR_ID
#define SENSORGRID_SENSOR_ID 2
#endif
static const uint8_t SENSOR_ID = SENSORGRID_SENSOR_ID;

static const int FIXED_CHANNEL = 1;
static const unsigned long SAMPLE_INTERVAL_MS = 100;

namespace crt
{
	SensorNode sensorNode(SENSOR_ID, FIXED_CHANNEL, SAMPLE_INTERVAL_MS);
}

void setup()
{
	ESP_LOGI("main", "=== SENSOR NODE v7 ===");
	crt::sensorNode.init();
}

void loop()
{
	crt::sensorNode.update();
}
