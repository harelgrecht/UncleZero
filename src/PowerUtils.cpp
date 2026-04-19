#include "../include/PowerUtils.h"
#include "../include/Config.h"
#include "../include/TankLogic.h"
#include <WiFi.h>
#include <time.h>
#include "driver/gpio.h"

void syncNtpTime() {
    Serial.println("Syncing time from NTP servers...");
    
    Serial.print("DNS IP: ");
    Serial.println(WiFi.dnsIP());

    IPAddress timeServerIP;
    if (WiFi.hostByName("pool.ntp.org", timeServerIP)) {
        Serial.print("SUCCESS: DNS resolved pool.ntp.org to ");
        Serial.println(timeServerIP);
    } else {
        Serial.println("ERROR: DNS failed! No active internet connection or DNS issue on this WiFi network.");
    }

    configTzTime("IST-2IDT,M3.4.4/26,M10.5.0", "pool.ntp.org", "time.nist.gov", "time.google.com");
    
    delay(2000); 

    struct tm timeinfo;
    int retry = 0;
    
    while (!getLocalTime(&timeinfo, 5000) && retry < 4) { 
        Serial.println("Waiting for NTP time sync...");
        retry++;
    }
    
    if (retry == 4) {
        Serial.println("Failed to obtain time from NTP.");
        return;
    }
    Serial.println(&timeinfo, "Current Real Time: %A, %d/%m/%Y %H:%M:%S");
}