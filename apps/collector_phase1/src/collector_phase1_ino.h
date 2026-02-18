// by Marius Versteegen, 2025

#pragma once
#include <Arduino.h>
#include "crt_CollectorNode.h"

static const char* AP_SSID = "SCOLIOSE";
static const char* AP_PASS = "scoliose";
static const int AP_CHANNEL = 1;

namespace crt
{
	CollectorNode collectorNode(AP_SSID, AP_PASS, AP_CHANNEL);
}

void setup()
{
	ESP_LOGI("main", "=== COLLECTOR PHASE 1 ===");
	crt::collectorNode.init();
}

void loop()
{
	crt::collectorNode.handleClient();
}
