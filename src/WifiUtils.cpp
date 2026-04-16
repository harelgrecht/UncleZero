#include <WiFi.h>
#include "../include/WifiUtils.h"
#include "../include/Config.h"
#include "../include/env.h"

void wifiScanNetworks() {   
    Serial.println("\r\n--- WiFi Scan ---");

    WiFi.disconnect(true, true);
    delay(100);
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    delay(100);

    int networkCount = WiFi.scanNetworks();
    
    if (networkCount == 0) {
        Serial.println("0 networks found.");
    } else if (networkCount > 0) {
        Serial.print(networkCount);
        Serial.println(" networks found:");
        Serial.printf("%d networks found:\r\n", networkCount);
        for (int i = 0; i < networkCount; ++i) {
            Serial.printf("[%d] %s | %d dBm\r\n", i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i));
            delay(10);
        }
    }
    WiFi.scanDelete();
    Serial.println("-----------------");
}

void wifiSetUp() {
    wifiScanNetworks();

    Serial.printf("\r\nConnecting to %s\r\n", wifiSsid);

    WiFi.disconnect(true, true);
    delay(1000); 

    WiFi.mode(WIFI_STA);
    WiFi.setTxPower(WIFI_POWER_11dBm); // Stable mid-range power
    WiFi.begin(wifiSsid, wifiPassword);

    int connectionAttempts = 0;
    bool isLedActive = false;
    
    while (WiFi.status() != WL_CONNECTED && connectionAttempts < 20) {
        delay(500);
        Serial.print(".");
        isLedActive = !isLedActive;
        digitalWrite(statusLedPin, isLedActive ? HIGH : LOW);
        connectionAttempts++;
    }

    digitalWrite(statusLedPin, ledOff); 

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected!");
        Serial.print("IP address/Su: ");
        Serial.println(WiFi.localIP());
        Serial.print("Subnet Mask: ");
        Serial.println(WiFi.subnetMask());
    } else {
        Serial.println("\r\nConnection Failed! Restarting...");
        delay(3000);
        ESP.restart();
    }
}

void monitorWiFiConnection() {
    static unsigned long lastCheckTime = 0;
    const unsigned long interval = 10000;

    if(millis() - lastCheckTime >= interval) {
        lastCheckTime = millis();

        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("Connection lost! Reconnecting...\tWiFi status: " + String(WiFi.status()));
            wifiSetUp();
        } else {
            Serial.printf("WiFi Stable | RSSI: %d dBm\r\n", WiFi.RSSI());
        }
    }
}