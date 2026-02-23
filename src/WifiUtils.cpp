#include "../include/WifiUtils.h"

const char* ssid = "Guy";
const char* password = "0542229097";

void wifiScanNetworks() {   
    Serial.println("\n--- WiFi Scan ---");

    WiFi.disconnect(true, true);
    delay(100);
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    delay(100);

    int n = WiFi.scanNetworks();
    
    if (n == 0) {
        Serial.println("0 networks found.");
    } else if (n > 0) {
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
    Serial.println("-----------------");
}

void wifiSetUp() {
    wifiScanNetworks();

    Serial.print("\nConnecting to ");
    Serial.println(ssid);

    // Reset radio before connecting
    WiFi.disconnect(true, true);
    delay(1000); 

    WiFi.mode(WIFI_STA);
    WiFi.setTxPower(WIFI_POWER_8_5dBm); // Prevent brownout
    WiFi.begin(ssid, password);

    int attempts = 0;
    bool ledState = false;
    
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        ledState = !ledState;
        digitalWrite(STATUS_LED, ledState ? HIGH : LOW);
        attempts++;
    }

    digitalWrite(STATUS_LED, HIGH); // LED off

    if (WiFi.status() == WL_CONNECTED) {
        WiFi.setTxPower(WIFI_POWER_19_5dBm); // Restore TX power
        Serial.println("\nWiFi connected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\nConnection Failed! Restarting...");
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
            Serial.println("Connection lost! Reconnecting...");
            WiFi.disconnect();
            WiFi.reconnect();
        } else {
            Serial.print("WiFi Stable | RSSI: ");
            Serial.println(WiFi.RSSI());
        }
    }
}