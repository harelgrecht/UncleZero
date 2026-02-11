#ifndef GAS_UTILS_H
#define GAS_UTILS_H

#include <Arduino.h>


#define STATUS_LED 8
#define SENSOR_PIN 10
 #define TANK_FULL HIGH // circut open 
 #define TANK_EMPTY LOW // circut close 

extern bool messageSent; 

void setupWiFi();
void setupEmail();
void sendEmail(String subject, String messageText);

#endif