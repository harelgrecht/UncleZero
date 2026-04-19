#include <Arduino.h>
#include "../include/Config.h"
#include "../include/WifiUtils.h"
#include "../include/TankLogic.h"
#include "../include/PowerUtils.h"
#include "../include/ConfigMode.h"

volatile bool isSensor1Triggered = false;
volatile bool isSensor2Triggered = false;
String staSSID;
String staPass;
unsigned long lastReport  = 0;
unsigned long lastWifiTry;


void reportTankStatusToServer(const String& tankName, const String& status) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[HTTP] Not connected to WiFi, skipping report.");
        return;
    }

    JsonDocument doc;
    String deviceId = getRestaurantName();
    if (deviceId.isEmpty()) deviceId = resturantID;
    doc["deviceId"] = deviceId;
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
    http.addHeader("X-Api-Key", "SNxwVBXzey1tPVAzGDdatR2EIOw3CY57");

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

void checkTankStatus() {
    int state1 = digitalRead(sensor1Pin);
    int state2 = digitalRead(sensor2Pin);

    updateAlertLed();
    reportTankStatusToServer("Tank 1", state1 == tankEmptyState ? "EMPTY" : "FULL");
    reportTankStatusToServer("Tank 2", state2 == tankEmptyState ? "EMPTY" : "FULL");
}

void setup() {
    Serial.begin(115200);
    delay(3000);
    pinMode(sensor1Pin, INPUT_PULLUP);
    pinMode(sensor2Pin, INPUT_PULLUP);
    pinMode(sensor1Gnd, OUTPUT);
    digitalWrite(sensor1Gnd, LOW);
    pinMode(sensor2Gnd, OUTPUT);
    digitalWrite(sensor2Gnd, LOW);
    pinMode(statusLedPin, OUTPUT);
    digitalWrite(statusLedPin, ledOff);
    Serial.println("\r\n[BOOT] Starting up...");

    // 1. Connect home WiFi as STA (normal STA mode)
    String nvsSsid, nvsPass;
    bool hasStored = loadWifiCredentials(nvsSsid, nvsPass);
    staSSID = hasStored ? nvsSsid.c_str() : wifiSsid;
    staPass = hasStored ? nvsPass.c_str() : wifiPassword;

    Serial.printf("[WIFI] Connecting to: %s\r\n", staSSID);
    WiFi.mode(WIFI_AP_STA);
    WiFi.setTxPower(WIFI_POWER_11dBm); // Stable mid-range power
    WiFi.begin(staSSID, staPass);
    int att = 0;
    while (WiFi.status() != WL_CONNECTED && att < 40) { delay(500); att++; }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[WIFI] Connected! IP: %s  ch%d\r\n",
                        WiFi.localIP().toString().c_str(), WiFi.channel());
        if (Settings::currentWakeupMode == MODE_SPECIFIC_TIME) syncNtpTime();
    } else {
        Serial.printf("[WIFI] Home WiFi unavailable (status=%d)\r\n", WiFi.status());
    }

    // 2. Start AP — ESP32 auto-matches AP channel to the connected STA channel
    startAlwaysOnServer(1);  // channel arg is overridden by STA channel when connected
}

void loop() {
    handleAlwaysOnClients();
    // Retry WiFi every 60 s when disconnected
    if (WiFi.status() != WL_CONNECTED && millis() - lastWifiTry > 60000) {
        lastWifiTry = millis();
        Serial.printf("[WIFI] Retrying: %s\r\n", staSSID);
        WiFi.begin(staSSID, staPass);
        int r = 0;
        while (WiFi.status() != WL_CONNECTED && r < 20) {
            handleAlwaysOnClients(); delay(500); r++;
        }
        if (WiFi.status() == WL_CONNECTED)
            Serial.printf("[WIFI] Reconnected! IP: %s\r\n", WiFi.localIP().toString().c_str());
    }
    if (millis() - lastReport > 30000) {
        lastReport = millis();
        checkTankStatus();
    }
    delay(5);
}