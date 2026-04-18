#pragma once
#include <Arduino.h>

// ==========================================
// NVS Keys
// ==========================================
#define NVS_NAMESPACE         "dodzero"
#define NVS_KEY_SSID          "wifi_ssid"
#define NVS_KEY_PASS          "wifi_pass"
#define NVS_KEY_WEB_ALWAYS_ON "web_always_on"

// AP hotspot name (open network — no password needed)
#define CONFIG_AP_SSID    "DodZero-Setup"

// mDNS hostname → http://dodzero.local
#define CONFIG_MDNS_HOST  "dodzero"

// enterConfigMode() reboots after this period of inactivity
#define CONFIG_MODE_TIMEOUT_MS  600000UL  // 10 minutes

// ==========================================
// Public functions
// ==========================================

/**
 * Blocks indefinitely, serving the WiFi config web portal.
 * Triggered by short-pressing the devMode pin at boot.
 * Reboots automatically after CONFIG_MODE_TIMEOUT_MS of inactivity.
 *
 * Access:
 *   Connect to "DodZero-Setup" WiFi → captive portal auto-opens
 *   Or navigate to http://192.168.4.1
 *   Also http://dodzero.local if stored WiFi connects
 */
void enterConfigMode();

/**
 * Reads WiFi credentials stored by the config portal from NVS.
 * Returns true when valid credentials are present.
 */
bool loadWifiCredentials(String& ssid, String& password);

/**
 * Returns true if the "always-on web portal" setting is enabled in NVS.
 */
bool isWebAlwaysOn();

/**
 * Starts the AP + web server without blocking.
 * Call once after WiFi connects in normal boot when isWebAlwaysOn() is true.
 * The AP ("DodZero-Setup") stays active so the device is always
 * reachable for reconfiguration even if the home WiFi changes.
 */
void startAlwaysOnServer(int apChannel = 1);

/**
 * Process pending web and DNS requests.
 * Call this every iteration of the main monitoring loop.
 */
void handleAlwaysOnClients();
