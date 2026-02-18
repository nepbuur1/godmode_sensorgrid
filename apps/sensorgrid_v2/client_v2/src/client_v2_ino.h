// by Marius Versteegen, 2025

#pragma once
#include <Arduino.h>
#include "crt_ClientNode.h"

static const char* WIFI_SSID = "SCOLIOSE";
static const char* WIFI_PASS = "scoliose";
static const char* SERVER_IP = "192.168.4.1";
static const uint16_t SERVER_PORT = 80;

namespace crt
{
	ClientNode clientNode(WIFI_SSID, WIFI_PASS, SERVER_IP, SERVER_PORT);
}

void setup()
{
	ESP_LOGI("main", "=== CLIENT NODE v2 ===");
	crt::clientNode.init();
}

void loop()
{
	crt::clientNode.update();
}
