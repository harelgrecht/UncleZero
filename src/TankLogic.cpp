#include "../include/TankLogic.h"
#include "../include/Config.h"

void updateAlertLed() {
    int state1 = digitalRead(sensor1Pin);
    int state2 = digitalRead(sensor2Pin);

    if (state1 == tankEmptyState || state2 == tankEmptyState) {
        digitalWrite(statusLedPin, ledOn);
    } else {
        digitalWrite(statusLedPin, ledOff);
    }
}

void processRoutineReport() {
    int state1 = digitalRead(sensor1Pin);
    int state2 = digitalRead(sensor2Pin);
    
    updateAlertLed();

    Serial.println("\r\n--- Routine Status Report ---");
    Serial.println("Tank 1: " + String(state1 == tankEmptyState ? "EMPTY" : "FULL"));
    Serial.println("Tank 2: " + String(state2 == tankEmptyState ? "EMPTY" : "FULL"));
    Serial.println("---------------------------\r\n");
}

void processTankEvent() {
    int state1 = digitalRead(sensor1Pin);
    int state2 = digitalRead(sensor2Pin);
    
    updateAlertLed();

    Serial.println("\r\n--- URGENT: TANK ALERT ---");
    if (state1 == tankEmptyState) Serial.println("EVENT: Tank 1 is EMPTY!\n\r");
    if (state2 == tankEmptyState) Serial.println("EVENT: Tank 2 is EMPTY!\n\r");
    Serial.println("---------------------------\r\n");
}