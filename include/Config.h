#pragma once
#include <Arduino.h>

// ==========================================
// Hardware Pins & States
// ==========================================
constexpr uint8_t statusLedPin = 8;
constexpr uint8_t sensor1Pin = 0;
constexpr uint8_t sensor1Gnd = 1;
constexpr uint8_t sensor2Pin = 2; 
constexpr uint8_t sensor2Gnd = 3;

constexpr uint8_t tankFullState = HIGH;  // Circuit open 
constexpr uint8_t tankEmptyState = LOW;  // Circuit close 

constexpr uint8_t ledOn = LOW;        // Active Low LED    
constexpr uint8_t ledOff = HIGH;

// ==========================================
// User Settings (Power & Wakeup)
// ==========================================
enum WakeupMode {
    MODE_INTERVAL,      
    MODE_SPECIFIC_TIME 
};

namespace Settings {
    constexpr WakeupMode currentWakeupMode = MODE_INTERVAL; 
    
    constexpr uint32_t intervalHours = 1; 
    
    constexpr uint8_t targetHour = 8;
    constexpr uint8_t targetMinute = 0; 
}