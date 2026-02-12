#include <Arduino.h>
#include "../include/WifiUtils.h"
#include "../include/MailUtils.h"




volatile bool sensorIrq = false;

void IRAM_ATTR isrChange() {
    sensorIrq = true;
}

void checkTankStatus() {
    int sensorState = digitalRead(SENSOR_PIN);

    if (sensorState == TANK_EMPTY) {
        digitalWrite(STATUS_LED, LED_ON);
        Serial.println("Tank Empty - Low Pressure");
        sendEmail("URGENT: Gas Tank Empty!", "The gas tank is empty. Please order a replacement.");
    } 
    else { 
        digitalWrite(STATUS_LED, LED_OFF);
        Serial.println("Tank Full - Pressure OK");
    }
}

void onBootTankStatus() {
    checkTankStatus();
    Serial.println("onBootTankStatus");
    sendEmail("Inital: First check on boot", "The gas tank is ...");
}

void setup() {
    Serial.begin(115200);
    delay(3000);

    pinMode(SENSOR_PIN, INPUT_PULLUP);
    pinMode(STATUS_LED, OUTPUT);
    digitalWrite(STATUS_LED, LED_OFF);

    wifiSetUp();
    delay(1000);
    mailSetUp();

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