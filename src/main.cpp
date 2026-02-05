#include <Arduino.h>


#define LOW_GAS HIGH
#define HIGH_GAS LOW
#define statusLed 8
#define sensorIn 10


void setup() {
    pinMode(sensorIn, INPUT);
    pinMode(statusLed, OUTPUT);
}

void loop() {
    if(digitalRead(statusLed) == HIGH_GAS) {
        digitalWrite(statusLed, LOW);  
    } else {
        digitalWrite(statusLed, HIGH); 
    }
}

