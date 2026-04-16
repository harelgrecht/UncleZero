#include <Arduino.h>
#include "../include/Config.h"
#include "../include/WifiUtils.h"
#include "../include/TankLogic.h"
#include "../include/PowerUtils.h"

volatile bool isSensor1Triggered = false;
volatile bool isSensor2Triggered = false;

void IRAM_ATTR handleSensor1Interrupt() {
    isSensor1Triggered = true;
}

void IRAM_ATTR handleSensor2Interrupt() {
    isSensor2Triggered = true;
}

void updateAlertLed() {
    int state1 = digitalRead(sensor1Pin);
    int state2 = digitalRead(sensor2Pin);

    if (state1 == tankEmptyState || state2 == tankEmptyState) {
        digitalWrite(statusLedPin, ledOn);
    } else {
        digitalWrite(statusLedPin, ledOff);
    }
}


void reportTankStatusToServer(const String& tankName, const String& status) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[HTTP] Not connected to WiFi, skipping report.");
        return;
    }

    JsonDocument doc;
    doc["deviceId"] = resturantID;
    doc["rssi"]     = WiFi.RSSI();
    doc["uptime"]   = millis() / 1000;
    
    JsonArray sensors = doc["sensors"].to<JsonArray>();
    JsonObject s1 = sensors.add<JsonObject>();
    s1["id"]     = 1;
    s1["name"]   = "Tank 1";
    s1["status"] = (tankName == "Tank 1") ? status : (digitalRead(sensor1Pin) == tankEmptyState ? "EMPTY" : "OK");

    JsonObject s2 = sensors.add<JsonObject>();
    s2["id"]     = 2;
    s2["name"]   = "Tank 2";
    s2["status"] = (tankName == "Tank 2") ? status : (digitalRead(sensor2Pin) == tankEmptyState ? "EMPTY" : "OK");
    
    String payload;
    serializeJson(doc, payload);
    
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");
    
    int httpCode = http.POST(payload);
    
    if (httpCode > 0) {
        Serial.printf("[HTTP] POST %s -> %d\n", serverUrl, httpCode);
    } else {
        Serial.printf("[HTTP] POST failed: %s\n", http.errorToString(httpCode).c_str());
    }
    
    http.end();
}

void processTankEvent(uint8_t pin, const String& tankName) {
    int currentState = digitalRead(pin);
    updateAlertLed(); 

    String status = (currentState == tankEmptyState) ? "EMPTY" : "OK";

    if (currentState == tankEmptyState) {
        Serial.println("Tank Empty - Low Pressure: " + tankName);
    } else {
        Serial.println("Tank Full - Pressure OK: " + tankName);
    }

    reportTankStatusToServer(tankName, status);
}

void verifyInitialTankStatus() {    
    int state1 = digitalRead(sensor1Pin);
    int state2 = digitalRead(sensor2Pin);
    
    updateAlertLed();

    String bootMessage = "System booted successfully.\n\n";
    bootMessage += "Tank 1: " + String(state1 == tankEmptyState ? "EMPTY" : "FULL") + "\n";
    reportTankStatusToServer("Tank 1", state1 == tankEmptyState ? "EMPTY" : "FULL");
    bootMessage += "Tank 2: " + String(state2 == tankEmptyState ? "EMPTY" : "FULL") + "\n";
    reportTankStatusToServer("Tank 2", state2 == tankEmptyState ? "EMPTY" : "FULL");

    Serial.println("Boot status:");
    Serial.println(bootMessage);
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    pinMode(sensor1Pin, INPUT_PULLUP);
    pinMode(sensor2Pin, INPUT_PULLUP);
    pinMode(devModePin, INPUT_PULLUP);
    pinMode(statusLedPin, OUTPUT);
    digitalWrite(statusLedPin, ledOff);

    if (digitalRead(devModePin) == LOW) {
        Serial.println("\r\n[MAINTENANCE MODE] firmware update pin is LOW!");
        Serial.println("System is staying awake endlessly. You can upload firmware now!");
        digitalWrite(statusLedPin, ledOn); 
        while (true) {
            delay(100);
        }
    }

    wifiSetUp();
    
    if (Settings::currentWakeupMode == MODE_SPECIFIC_TIME) {
        syncNtpTime();
    }

    printWakeupReason();

    goToSleep();
}

void loop() {
    // Left empty intentionally. System operates on Deep Sleep architecture.
}