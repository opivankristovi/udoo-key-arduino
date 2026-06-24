# RepeaterNodeESP32

WiFi NAT repeater + MQTT hub + captive-portal web page — the ESP32 half of the Udoo Key Repeater Node pair.

Must be used together with **[rp2040/RepeaterNodeRP2040](../../rp2040/RepeaterNodeRP2040/)**.

## What it does

- Connects to an upstream Wi-Fi network (STA) and creates a local access point (AP)
- Enables NAT so devices connected to the AP can reach the internet (true repeater)
- Receives JSON sensor packets from the RP2040 over the on-board UART bridge
- Publishes each sensor value to MQTT
- Subscribes to output command topics and forwards commands to the RP2040
- **Captive-portal web page** for runtime configuration — no re-flashing needed
- **Home Assistant auto-discovery** — publishes retained MQTT config topics on connect
- **NTP time sync** + **per-output clock schedules** (turn output on/off at configured times)

## Board & toolchain

- **Board:** ESP32 Dev Module
- **Arduino IDE** with the ESP32 board package (Espressif Systems)
- **Compiles on:** Arduino-ESP32 core 2.x and 3.x (NAPT include is `#if`-switched)

## Required libraries

PubSubClient · **ArduinoJson v7** (v6 will not compile — uses `JsonDocument`)

`WebServer`, `DNSServer`, `Preferences` ship with the ESP32 Arduino core — no extra install needed.

## Configuration

**Flash first, configure via web portal** — no need to edit `config.h`.

1. Flash `RepeaterNodeESP32.ino` (board: ESP32 Dev Module).
2. Join Wi-Fi **UdooKey-AP** / **repeater1**.
3. Browse to **http://192.168.4.1** — the captive portal opens automatically.
4. Set your upstream Wi-Fi credentials, MQTT broker, NTP server, timezone, and optional clock schedules.
5. Save — the device reboots and connects.

Settings are stored in NVS (flash). `config.h` only provides first-boot defaults used before you save from the portal.

### Web portal tabs

| Tab | What you configure |
|-----|--------------------|
| Network | STA (upstream) SSID/password, AP SSID/password |
| MQTT | Broker host, port, credentials, client ID, topic base, HA discovery |
| System | NTP server, POSIX timezone string |
| Schedules | Per-output on/off times and ON level (0-255) |

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
