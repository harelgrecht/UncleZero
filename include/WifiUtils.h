#ifndef WIFIUTILS_H
#define WIFIUTILS_H


#include <Arduino.h>
#include <WiFi.h>
#include "../include/env.h"

// Hardware Pins & States
constexpr uint8_t statusLedPin = 8;
constexpr uint8_t sensorPin = 10;

constexpr uint8_t tankFullState = HIGH; // Circuit open 
constexpr uint8_t tankEmptyState = LOW; // Circuit close 

constexpr uint8_t ledOn = LOW;          // Active Low LED
constexpr uint8_t ledOff = HIGH;

void wifiScanNetworks();
void wifiSetUp();
void monitorWiFiConnection();

#endif