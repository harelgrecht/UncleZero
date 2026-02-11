#ifndef GAS_UTILS_H
#define GAS_UTILS_H

#include <Arduino.h>
#include <WiFi.h>

#define STATUS_LED 8
#define SENSOR_PIN 10
 #define TANK_FULL HIGH // circut open 
 #define TANK_EMPTY LOW // circut close 

void wifiScanNetworks();
void wifiSetUp();
void monitorWiFiConnection();

#endif