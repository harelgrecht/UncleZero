#include <Arduino.h>
#include "../include/utils.h"

volatile bool sensorIrq = false;

void IRAM_ATTR irqChange() {
    sensorIrq = true;
}

void isrTankStatus() {
    int sensorState = digitalRead(SENSOR_PIN);

    if (sensorState == TANK_EMPTY) {
        digitalWrite(STATUS_LED, LOW);
        Serial.println("Tank Empty - Low Pressure");
    } 
    else { 
        digitalWrite(STATUS_LED, HIGH);
        Serial.println("Tank Full - Pressure OK");
    }
}

void onBootTankStatus() {
    isrTankStatus();
}

void setup() {
    Serial.begin(115200);
    delay(3000);
    pinMode(SENSOR_PIN, INPUT_PULLUP);
    pinMode(STATUS_LED, OUTPUT);

    wifiSetUp();

    onBootTankStatus();

    attachInterrupt(digitalPinToInterrupt(SENSOR_PIN), irqChange, CHANGE);
}

void loop() {
    monitorWiFiConnection();

    if (sensorIrq) {
        delay(50); // Simple debounce
        isrTankStatus(); // Handle the event
        sensorIrq = false; // Reset flag
    }
}