#include <Arduino.h>
#include "../include/WifiUtils.h"
#include "../include/MailUtils.h"



volatile bool isSensorTriggered = false;

void IRAM_ATTR handleSensorInterrupt() {
    isSensorTriggered = true;
}

void checkTankStatus() {
    int sensorState = digitalRead(sensorPin);

    if (sensorState == tankEmptyState) {
        digitalWrite(statusLedPin, ledOn);
        Serial.println("Tank Empty - Low Pressure");
        sendEmail("URGENT: Gas Tank Empty!", "The gas tank is empty. Please order a replacement.");
    } 
    else { 
        digitalWrite(statusLedPin, ledOff);
        Serial.println("Tank Full - Pressure OK");
    }
}

void verifyInitialTankStatus() {    
    int sensorState = digitalRead(sensorPin);

    if (sensorState == tankEmptyState) {
        digitalWrite(statusLedPin, ledOn);
        Serial.println("Boot status: Tank Empty");
        sendEmail("Initial: First check on boot", "The gas tank is EMPTY. Please order a replacement.");
    } 
    else { 
        digitalWrite(statusLedPin, ledOff);
        Serial.println("Boot status: Tank Full");
        sendEmail("Initial: First check on boot", "The gas tank is FULL. Everything is OK.");
    }
}

void setup() {
    Serial.begin(115200);
    delay(3000);

    pinMode(sensorPin, INPUT_PULLUP);
    pinMode(statusLedPin, OUTPUT);
    digitalWrite(statusLedPin, ledOff);

    wifiSetUp();
    delay(1000);
    mailSetUp();

    verifyInitialTankStatus();

    attachInterrupt(digitalPinToInterrupt(sensorPin), handleSensorInterrupt, CHANGE);
}

void loop() {
    monitorWiFiConnection();

    if (isSensorTriggered) {
        delay(50); 
        checkTankStatus(); 
        isSensorTriggered = false; 
    }
}