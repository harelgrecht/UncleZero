#include <Arduino.h>
#include "../include/WifiUtils.h"

volatile bool sensorIrq = false;

void IRAM_ATTR isrChange() {
    sensorIrq = true;
}

void checkTankStatus() {
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
    checkTankStatus();
}

void setup() {
    Serial.begin(115200);
    delay(3000);
    pinMode(SENSOR_PIN, INPUT_PULLUP);
    pinMode(STATUS_LED, OUTPUT);

    wifiSetUp();

    onBootTankStatus();

    attachInterrupt(digitalPinToInterrupt(SENSOR_PIN), isrChange, CHANGE);
}

void loop() {
    monitorWiFiConnection();

    if (sensorIrq) {
        delay(50); 
        checkTankStatus(); 
        sensorIrq = false; 
    }
}