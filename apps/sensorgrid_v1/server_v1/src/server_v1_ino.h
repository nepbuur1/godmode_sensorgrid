// by Marius Versteegen, 2025

#pragma once
#include <Arduino.h>
#include "crt_ServerNode.h"

static const char* AP_SSID = "SCOLIOSE";
static const char* AP_PASS = "scoliose";
static const int AP_CHANNEL = 1;

namespace crt
{
	ServerNode serverNode(AP_SSID, AP_PASS, AP_CHANNEL);
}

void setup()
{
	ESP_LOGI("main", "=== SERVER NODE v1 ===");
	crt::serverNode.init();
}

void loop()
{
	crt::serverNode.handleClient();
}
