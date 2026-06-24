#pragma once

// First-boot defaults — used only when NVS is empty (fresh flash or factory reset).
// All settings are overridable at runtime via the web portal at 192.168.4.1.
// After you save from the portal, this file is no longer consulted.

// --- Upstream Wi-Fi (the network to extend) ---
#define WIFI_STA_SSID   "your_upstream_ssid"
#define WIFI_STA_PASS   "your_upstream_password"

// --- Repeater access point created by this device ---
#define WIFI_AP_SSID    "UdooKey-AP"
#define WIFI_AP_PASS    "repeater1"     // minimum 8 characters

// --- MQTT broker ---
#define MQTT_HOST       "192.168.1.100"
#define MQTT_PORT       1883
#define MQTT_USER       ""              // leave blank for no authentication
#define MQTT_PASS       ""
#define MQTT_CLIENT     "udookey"       // must be unique per device on your broker

// --- MQTT topic base ---
// Every topic is  <TOPIC_BASE>/<subtopic>
#define TOPIC_BASE      "udookey"
