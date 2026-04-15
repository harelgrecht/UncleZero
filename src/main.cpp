#include <Arduino.h>
#include "../include/Config.h"
#include "../include/WifiUtils.h"
#include "../include/TankLogic.h"
#include "../include/PowerUtils.h"

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