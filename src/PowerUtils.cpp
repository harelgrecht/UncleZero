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

uint64_t calculateSleepSeconds() {
    if (Settings::currentWakeupMode == MODE_INTERVAL) {
        Serial.printf("Config: Wake up every %d hours.\r\n", Settings::intervalHours);
        return (uint64_t)Settings::intervalHours * 3600; 
    } 
    else {
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo)) {
            Serial.println("Warning: Time not synced! Defaulting to 12h sleep.");
            return 12 * 3600;
        }

        int currentTotalSeconds = (timeinfo.tm_hour * 3600) + (timeinfo.tm_min * 60) + timeinfo.tm_sec;
        int targetTotalSeconds = (Settings::targetHour * 3600) + (Settings::targetMinute * 60);

        int secondsToSleep = targetTotalSeconds - currentTotalSeconds;
        if (secondsToSleep <= 0) secondsToSleep += 24 * 3600;

        Serial.printf("Config: Wake up at %02d:%02d. Sleeping for %d seconds.\r\n", Settings::targetHour, Settings::targetMinute, secondsToSleep);
        return (uint64_t)secondsToSleep;
    }
}

void printWakeupReason() {
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason) {
    case ESP_SLEEP_WAKEUP_GPIO:
        Serial.println("Wakeup Reason: Tank sensor triggered! (GPIO Wakeup)"); 
        processTankEvent();
        break;
    case ESP_SLEEP_WAKEUP_TIMER:    
        Serial.println("Wakeup Reason: Scheduled Timer"); 
        processRoutineReport();
        break;
    default : 
        Serial.println("Wakeup Reason: Power on / Manual Reset"); 
        processRoutineReport(); 
        break;
  }
}

void goToSleep() {
    Serial.println("\r\nPreparing to sleep...");
    
    uint64_t sleepSeconds = calculateSleepSeconds();
    esp_sleep_enable_timer_wakeup(sleepSeconds * 1000000ULL); 
    
    gpio_set_pull_mode((gpio_num_t)sensor1Pin, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode((gpio_num_t)sensor2Pin, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode((gpio_num_t)devModePin, GPIO_PULLUP_ONLY);
    
    gpio_hold_en((gpio_num_t)sensor1Pin);
    gpio_hold_en((gpio_num_t)sensor2Pin);
    gpio_hold_en((gpio_num_t)devModePin);
    
    gpio_deep_sleep_hold_en(); 

    uint64_t bitmask = 0;
    
    if (digitalRead(sensor1Pin) == HIGH) {
        gpio_wakeup_enable((gpio_num_t)sensor1Pin, GPIO_INTR_LOW_LEVEL);
        bitmask |= (1ULL << sensor1Pin);
    }
    
    if (digitalRead(sensor2Pin) == HIGH) {
        gpio_wakeup_enable((gpio_num_t)sensor2Pin, GPIO_INTR_LOW_LEVEL);
        bitmask |= (1ULL << sensor2Pin);
    }
    
    gpio_wakeup_enable((gpio_num_t)devModePin, GPIO_INTR_LOW_LEVEL);
    bitmask |= (1ULL << devModePin);

    if (bitmask > 0) {
        esp_deep_sleep_enable_gpio_wakeup(bitmask, ESP_GPIO_WAKEUP_GPIO_LOW);
    }
    
    Serial.println("Going to sleep NOW. Zzz...\r\n");
    delay(200); 
    esp_deep_sleep_start();
}