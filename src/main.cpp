#include <Arduino.h>
#include "../include/Config.h"
#include "../include/WifiUtils.h"
#include "../include/TankLogic.h"
#include "../include/PowerUtils.h"
#include "../include/ConfigMode.h"

volatile bool isSensor1Triggered = false;
volatile bool isSensor2Triggered = false;

void IRAM_ATTR handleSensor1Interrupt() {
    isSensor1Triggered = true;
}

void IRAM_ATTR handleSensor2Interrupt() {
    isSensor2Triggered = true;
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
    http.addHeader("X-Api-Key", "H4v4HSc8qF-Vt7UaL6pqlF81IJOeFcmM");
    
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
        // Hold LOW for > 3 s → maintenance mode (firmware update)
        // Release within 3 s   → config mode (WiFi provisioning via USB/BLE)
        Serial.println("\r\n[BOOT] devMode pin is LOW - waiting 3 s to detect intent...");
        unsigned long holdStart = millis();
        bool released = false;
        while (millis() - holdStart < 3000) {
            if (digitalRead(devModePin) == HIGH) { released = true; break; }
            delay(50);
        }

        if (!released) {
            Serial.println("[MAINTENANCE MODE] Holding LOW - staying awake for firmware upload.");
            digitalWrite(statusLedPin, ledOn);
            while (true) { delay(100); }
        } else {
            // Short press → config mode (web portal, blocks until reboot)
            enterConfigMode();
        }
    }

    // ── Always-on mode ────────────────────────────────────────────────────────
    // Start the AP *first* (WIFI_AP_STA mode), then connect to home WiFi as STA.
    // This avoids a mode-switch after WiFi is already in WIFI_STA, which can
    // silently prevent softAP from becoming visible.
    if (isWebAlwaysOn()) {
        startAlwaysOnServer();   // brings up AP + web server in WIFI_AP_STA mode

        // Let AP stabilise for 3 s before the STA scan starts.
        // STA scanning temporarily suppresses AP beacon frames.
        for (int i = 0; i < 600; i++) { handleAlwaysOnClients(); delay(5); }

        // Try connecting to home WiFi (keeps web server alive during attempt)
        String nvsSsid, nvsPass;
        auto tryConnectWifi = [&]() {
            loadWifiCredentials(nvsSsid, nvsPass);
            const char* ssid = nvsSsid.length() > 0 ? nvsSsid.c_str() : wifiSsid;
            const char* pass = nvsPass.length() > 0  ? nvsPass.c_str()  : wifiPassword;
            Serial.printf("[WIFI] Connecting to: %s\r\n", ssid);
            WiFi.begin(ssid, pass);
            int att = 0;
            while (WiFi.status() != WL_CONNECTED && att < 20) {
                handleAlwaysOnClients(); delay(500); att++;
            }
            if (WiFi.status() == WL_CONNECTED) {
                Serial.printf("[WIFI] Connected! IP: %s\r\n", WiFi.localIP().toString().c_str());
                if (Settings::currentWakeupMode == MODE_SPECIFIC_TIME) syncNtpTime();
            } else {
                Serial.println("[WIFI] Home WiFi unavailable - AP still at 192.168.4.1");
            }
        };
        tryConnectWifi();

        verifyInitialTankStatus();
        unsigned long lastReport  = 0;
        unsigned long lastWifiTry = millis();
        while (true) {
            handleAlwaysOnClients();

            // Retry WiFi every 60 s when disconnected
            if (WiFi.status() != WL_CONNECTED && millis() - lastWifiTry > 60000) {
                lastWifiTry = millis();
                tryConnectWifi();
            }

            if (millis() - lastReport > 30000) {
                lastReport = millis();
                verifyInitialTankStatus();
            }
            delay(5);
        }
    }

    // ── Normal (deep-sleep) mode ───────────────────────────────────────────────
    // Try NVS credentials first, fall back to env.h defaults
    String nvsSsid, nvsPass;
    if (loadWifiCredentials(nvsSsid, nvsPass)) {
        Serial.printf("[WIFI] Using stored credentials for: %s\r\n", nvsSsid.c_str());
        WiFi.begin(nvsSsid.c_str(), nvsPass.c_str());
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) { delay(500); attempts++; }
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[WIFI] Stored credentials failed, falling back to env.h");
            wifiSetUp();
        } else {
            Serial.printf("[WIFI] Connected! IP: %s\r\n", WiFi.localIP().toString().c_str());
        }
    } else {
        wifiSetUp();
    }

    if (Settings::currentWakeupMode == MODE_SPECIFIC_TIME) {
        syncNtpTime();
    }

    printWakeupReason();

    goToSleep();
}

void loop() {
    // Left empty intentionally. System operates on Deep Sleep architecture.
}