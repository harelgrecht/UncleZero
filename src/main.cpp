#include <Arduino.h>
#include "../include/utils.h"

void setup() {
    Serial.begin(115200);

    pinMode(SENSOR_PIN, INPUT_PULLUP);
    pinMode(STATUS_LED, OUTPUT);

    //setupWiFi();
    setupEmail();
}

void loop() {
    int sensorState = digitalRead(SENSOR_PIN);

    if (sensorState == TANK_EMPTY) {
        digitalWrite(STATUS_LED, TANK_EMPTY);
        Serial.println("Tank Empty - Low Pressure");

        if (!messageSent) {
            Serial.println(">> Gas tank empty! Sending Email...");
            sendEmail("WARNING: Gas tank empty Detected!", "Alert: Low pressure detected.");
            messageSent = true;
        }
    } 
    else { // TANK_FULL
        digitalWrite(STATUS_LED, TANK_FULL);
        Serial.println("Tank Full - Pressure OK");

        if (messageSent) {
            Serial.println(">> System returned to normal state.");
            messageSent = false;
        }
    }
    
    delay(500);
}