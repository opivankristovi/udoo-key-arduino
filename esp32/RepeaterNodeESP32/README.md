# RepeaterNodeESP32

WiFi NAT repeater + MQTT hub — the ESP32 half of the Udoo Key Repeater Node pair.

Must be used together with **[rp2040/RepeaterNodeRP2040](../../rp2040/RepeaterNodeRP2040/)**.

## What it does

- Connects to an upstream Wi-Fi network (STA) and creates a local access point (AP)
- Enables NAT so devices connected to the AP can reach the internet (true repeater)
- Receives JSON sensor packets from the RP2040 over the on-board UART bridge
- Publishes each sensor value to MQTT
- Subscribes to output command topics and forwards commands to the RP2040

## Board & toolchain

- **Board:** ESP32 Dev Module
- **Arduino IDE** with the ESP32 board package (Espressif Systems)
- **Compiles on:** Arduino-ESP32 core 2.x and 3.x (NAPT include is `#if`-switched)

## Required libraries

PubSubClient · **ArduinoJson v7** (v6 will not compile — uses `JsonDocument`)

## Configuration

Edit **`config.h`** before flashing:

| Define | Purpose |
|--------|---------|
| `WIFI_STA_SSID` / `WIFI_STA_PASS` | Upstream network to extend |
| `WIFI_AP_SSID` / `WIFI_AP_PASS` | Name and password for the new AP (min 8 chars) |
| `MQTT_HOST` / `MQTT_PORT` | Broker address |
| `MQTT_USER` / `MQTT_PASS` | Broker credentials (leave blank if none) |
| `MQTT_CLIENT` | Client ID — must be unique per device on your broker |
| `TOPIC_BASE` | MQTT topic prefix (default `udookey`) |

## UART protocol

Both sketches use **9600 baud** on `Serial1`. Lines are newline-terminated.

| Direction | Format | Meaning |
|-----------|--------|---------|
| RP2040 → ESP32 | `S:{json}\n` | Sensor packet |
| ESP32 → RP2040 | `C:{"o":N,"v":V}\n` | Output command (N = 0-based index, V = 0-255) |

## MQTT topics

All topics are under `<TOPIC_BASE>/` (default `udookey/`).

| Topic | Direction | Notes |
|-------|-----------|-------|
| `status` | publish | `online` / `offline` (retained LWT) |
| `sensor/i2c_a/temperature` | publish | °C — only published if present in sensor packet |
| `sensor/i2c_a/humidity` | publish | % RH (BME280 only) |
| `sensor/i2c_a/pressure` | publish | hPa |
| `sensor/i2c_b/…` | publish | Same fields for I2C device B |
| `sensor/probe/temperature` | publish | °C |
| `sensor/probe/humidity` | publish | % RH (DHT11/DHT22 only) |
| `sensor/analog/1` | publish | Raw ADC 0-4095 |
| `sensor/analog/2` | publish | Raw ADC 0-4095 |
| `input/1/state` | publish | `ON` / `OFF` (retained) |
| `input/2/state` | publish | `ON` / `OFF` (retained) |
| `output/1/state` … `output/4/state` | publish | `ON` / `OFF` (retained) |
| `output/1/set` … `output/4/set` | subscribe | `ON` \| `OFF` \| `0-255` |

## Hardware pins (ESP32 side, fixed by Udoo Key PCB)

| Signal | GPIO |
|--------|------|
| UART RX (from RP2040) | 22 |
| UART TX (to RP2040) | 19 |
| Blue LED | 32 |
| Yellow LED | 33 |
