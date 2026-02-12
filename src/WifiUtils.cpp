#include "../include/WifiUtils.h"
#include <WiFi.h>

const char* ssid = "Harel’s iPhone";
const char* password = "0123456789";

const IPAddress primaryDNS(8, 8, 8, 8);
const IPAddress secondaryDNS(8, 8, 4, 4);

void wifiScanNetworks() {   
 Serial.println("\n--- WiFi Diagnostic Scan ---");

    WiFi.disconnect(true, true);
    delay(100);
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    delay(100);

    Serial.println("Scanning...");
    int n = WiFi.scanNetworks();
    
    if (n == 0) {
        Serial.println("Result: 0 networks found.");
    } 
    else if (n < 0) {
        Serial.print("Error Code: ");
        Serial.println(n);
    }
    else {
        Serial.print(n);
        Serial.println(" networks found:");
        for (int i = 0; i < n; ++i) {
            Serial.print("[");
            Serial.print(i + 1);
            Serial.print("] ");
            Serial.print(WiFi.SSID(i));
            Serial.print(" | ");
            Serial.print(WiFi.RSSI(i));
            Serial.println(" dBm");
            delay(10);
        }
    }
    WiFi.scanDelete();
    Serial.println("----------------------------");
}

void wifiSetUp() {
    wifiScanNetworks();

    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);

    if (!WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, primaryDNS, secondaryDNS)) {
        Serial.println("Failed to configure DNS!");
    }

    WiFi.begin(ssid, password);

    bool ledState = false;
    while (WiFi.status() != WL_CONNECTED) {
        delay(250);
        Serial.print(".");
        ledState = !ledState;
        digitalWrite(STATUS_LED, ledState ? LED_OFF : LED_ON);
    }
    digitalWrite(STATUS_LED, LED_OFF);
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

void monitorWiFiConnection() {
    static unsigned long lastCheckTime = 0;
    const unsigned long interval = 10000;

    if(millis() - lastCheckTime >= interval) {
        lastCheckTime = millis();

        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("Connection lost! Reconnecting...");
            WiFi.disconnect();
            WiFi.reconnect();
        } else {
            Serial.print("WiFi Stable | RSSI: ");
            Serial.println(WiFi.RSSI());
            
        }
    }
}