#include <Arduino.h>


#define LOW_GAS HIGH
#define HIGH_GAS LOW
#define statusLed 8
#define sensorPin 10


void setup() {
    pinMode(sensorPin, INPUT);
    pinMode(sensorPin, INPUT_PULLUP);
    pinMode(statusLed, OUTPUT);
    Serial.begin(115200);
}

void loop() {
    if(digitalRead(sensorPin) == LOW) {
        digitalWrite(statusLed, LOW);
        Serial.println("No Gas (Closed) - LED ON");
    } else {
        digitalWrite(statusLed, HIGH);
        Serial.println("Gas Detected (Open) - LED OFF");
    }
    delay(100);
}

