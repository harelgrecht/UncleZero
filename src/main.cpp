#include <Arduino.h>
#include "../include/WifiUtils.h"
#include "../include/MailUtils.h"

volatile bool isSensor1Triggered = false;
volatile bool isSensor2Triggered = false;

void IRAM_ATTR handleSensor1Interrupt() {
    isSensor1Triggered = true;
}

void IRAM_ATTR handleSensor2Interrupt() {
    isSensor2Triggered = true;
}

void updateAlertLed() {
    int state1 = digitalRead(sensor1Pin);
    int state2 = digitalRead(sensor2Pin);

    if (state1 == tankEmptyState || state2 == tankEmptyState) {
        digitalWrite(statusLedPin, ledOn);
    } else {
        digitalWrite(statusLedPin, ledOff);
    }
}

void processTankEvent(uint8_t pin, const String& tankName) {
    int currentState = digitalRead(pin);
    updateAlertLed(); 

    if (currentState == tankEmptyState) {
        Serial.println("Tank Empty - Low Pressure: " + tankName);
        String subject = "URGENT: " + tankName + " is Empty!";
        String body = "The " + tankName + " is currently empty. Please order a replacement.";
        sendEmail(subject, body);
    } 
    else { 
        Serial.println("Tank Full - Pressure OK: " + tankName);
    }
}

void verifyInitialTankStatus() {    
    int state1 = digitalRead(sensor1Pin);
    int state2 = digitalRead(sensor2Pin);
    
    updateAlertLed();

    String bootMessage = "System booted successfully.\n\n";
    bootMessage += "Tank 1: " + String(state1 == tankEmptyState ? "EMPTY" : "FULL") + "\n";
    bootMessage += "Tank 2: " + String(state2 == tankEmptyState ? "EMPTY" : "FULL") + "\n";
    
    Serial.println("Boot status:");
    Serial.println(bootMessage);
    sendEmail("Initial Check: System Online", bootMessage);
}

void setup() {
    Serial.begin(115200);
    delay(3000);

    pinMode(sensor1Pin, INPUT_PULLUP);
    pinMode(sensor2Pin, INPUT_PULLUP);
    
    pinMode(statusLedPin, OUTPUT);
    digitalWrite(statusLedPin, ledOff);

    wifiSetUp();
    delay(1000);
    mailSetUp();

    verifyInitialTankStatus();

    attachInterrupt(digitalPinToInterrupt(sensor1Pin), handleSensor1Interrupt, CHANGE);
    attachInterrupt(digitalPinToInterrupt(sensor2Pin), handleSensor2Interrupt, CHANGE);
}

void loop() {
    monitorWiFiConnection();

    if (isSensor1Triggered) {
        delay(50);
        processTankEvent(sensor1Pin, "Tank 1"); 
        isSensor1Triggered = false; 
    }

    if (isSensor2Triggered) {
        delay(50);
        processTankEvent(sensor2Pin, "Tank 2"); 
        isSensor2Triggered = false; 
    }
}