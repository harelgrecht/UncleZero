#include "../include/utils.h"
#include <WiFi.h>

const char* ssid = "LironWifi";
const char* password = "123456789";

void wifiScanNetworks() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    Serial.println("Starting network scan...");
    int wifiCount = WiFi.scanNetworks();
    
    if (wifiCount == 0) {
        Serial.println("No networks found");
    } else {
        Serial.print(wifiCount);
        Serial.println(" networks found:");
        for (int i = 0; i < wifiCount; ++i) {
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.print(WiFi.SSID(i));
            Serial.print(" (");
            Serial.print(WiFi.RSSI(i));
            Serial.print(")");
            Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
            delay(10);
        }
    }
    Serial.println("Scan done");
}

void wifiSetUp() {
    wifiScanNetworks();

    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

void monitorWiFiConnection() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Connection lost! Reconnecting...");
        WiFi.disconnect();
        WiFi.reconnect();
    } else {
        Serial.print("WiFi Stable | RSSI: ");
        Serial.println(WiFi.RSSI());
    }
}