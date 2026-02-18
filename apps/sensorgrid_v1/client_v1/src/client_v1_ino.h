// by Marius Versteegen, 2025

#pragma once
#include <Arduino.h>
#include "crt_ClientNode.h"

static const char* AP_SSID = "SCOLIOSE";
static const char* AP_PASS = "scoliose";
static const char* SERVER_IP = "192.168.4.1";
static const uint16_t SERVER_PORT = 80;

namespace crt
{
	ClientNode clientNode(AP_SSID, AP_PASS, SERVER_IP, SERVER_PORT);
}

void setup()
{
	ESP_LOGI("main", "=== CLIENT TEST NODE v1 ===");
	crt::clientNode.init();
}

void loop()
{
	crt::clientNode.update();
}
