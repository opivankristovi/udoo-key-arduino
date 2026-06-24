#pragma once

// =====================================================================
// USER CONFIGURATION — fill in before flashing the ESP32
// =====================================================================

// --- Upstream Wi-Fi (the network to extend) ---
#define WIFI_STA_SSID  "your_upstream_ssid"
#define WIFI_STA_PASS  "your_upstream_password"

// --- Repeater access point created by this device ---
#define WIFI_AP_SSID   "UdooKey-AP"
#define WIFI_AP_PASS   "repeater1"   // minimum 8 characters

// --- MQTT broker ---
#define MQTT_HOST      "192.168.1.x"
#define MQTT_PORT      1883
#define MQTT_USER      ""            // leave blank if no broker authentication
#define MQTT_PASS      ""
#define MQTT_CLIENT    "udookey"     // must be unique per device on your broker

// --- MQTT topic base ---
// Every topic is published as  <TOPIC_BASE>/<subtopic>
#define TOPIC_BASE     "udookey"
