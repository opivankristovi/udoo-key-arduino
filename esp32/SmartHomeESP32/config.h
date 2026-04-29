#pragma once

// Fill in these values before flashing.
// Consider adding config.h to .gitignore to avoid committing credentials.

// --- Wi-Fi ---
#define WIFI_SSID     "your_ssid_here"
#define WIFI_PASSWORD "your_password_here"

// --- MQTT broker ---
#define MQTT_HOST    "192.168.1.x"   // local broker IP or hostname
#define MQTT_PORT    1883
#define MQTT_CLIENT  "udookey"        // change if running multiple boards

// --- MQTT topics ---
#define TOPIC_TEMP   "udookey/sensor/temperature"
#define TOPIC_HUM    "udookey/sensor/humidity"
#define TOPIC_AUDIO  "udookey/audio/event"
#define TOPIC_STATUS "udookey/status"
