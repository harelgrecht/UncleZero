#include <Arduino.h>
#include "../include/utils.h"

void setup() {
    Serial.begin(115200);
    delay(5000);
    pinMode(SENSOR_PIN, INPUT_PULLUP);
    pinMode(STATUS_LED, OUTPUT);

    //wifiSetUp();

}

void loop() {
    monitorWiFiConnection();
    int sensorState = digitalRead(SENSOR_PIN);

    if (sensorState == TANK_EMPTY) {
        digitalWrite(STATUS_LED, LOW);
        Serial.println("Gas tank Empty - Low Pressure");
    } 
    else { // TANK_FULL
        digitalWrite(STATUS_LED, HIGH);
        Serial.println("Tank Full - Pressure OK");
    }
    
    delay(1000);
}