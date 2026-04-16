#ifndef WIFIUTILS_H
#define WIFIUTILS_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "../include/env.h"

void wifiScanNetworks();
void wifiSetUp();
void monitorWiFiConnection();

#endif