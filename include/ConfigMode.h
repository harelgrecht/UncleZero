#pragma once
#include <Arduino.h>

// ==========================================
// NVS Keys for persisted WiFi credentials
// ==========================================
#define NVS_NAMESPACE  "dodzero"
#define NVS_KEY_SSID   "wifi_ssid"
#define NVS_KEY_PASS   "wifi_pass"

// AP hotspot name (open network — no password needed)
#define CONFIG_AP_SSID  "DodZero-Setup"

// mDNS hostname → http://dodzero.local
#define CONFIG_MDNS_HOST  "dodzero"

// Auto-reboot after this period of inactivity in config mode
#define CONFIG_MODE_TIMEOUT_MS  600000UL  // 10 minutes

// ==========================================
// Public functions
// ==========================================

/**
 * Blocks indefinitely, serving a web-based WiFi config portal.
 *
 * Access points:
 *   - Connect to "DodZero-Setup" WiFi → browser auto-opens (captive portal)
 *     or navigate to http://192.168.4.1
 *   - If stored WiFi credentials exist, also accessible at
 *     http://dodzero.local (or the device IP) from your existing network
 *
 * Reboots automatically after CONFIG_MODE_TIMEOUT_MS of inactivity.
 */
void enterConfigMode();

/**
 * Reads WiFi credentials stored by the config portal from NVS.
 * Returns true when valid credentials are present.
 */
bool loadWifiCredentials(String& ssid, String& password);
