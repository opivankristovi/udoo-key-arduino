# Udoo Key Arduino Repository

## Board overview

The **Udoo Key** is a dual-MCU development board:
- **ESP32** — handles Wi-Fi, Bluetooth, I2S microphone (Pro), IMU/MPU6500 (Pro), and on-board UART to RP2040
- **RP2040** — general GPIO, sensors, and on-board UART to ESP32

The **Udoo Key Pro** adds an on-board SPK0838HT4H-1 I2S digital microphone and an MPU6500 IMU, both wired to the ESP32.

## Repository structure

```
esp32/      — sketches to be flashed to the ESP32
rp2040/     — sketches to be flashed to the RP2040
REFERENCE.md — quick-reference pinout tables
```

## Arduino IDE board selection

| MCU   | Board to select in Arduino IDE |
|-------|-------------------------------|
| ESP32 | ESP32 Dev Module               |
| RP2040 | Raspberry Pi Pico Module      |

Board manager URLs (add under File → Preferences):
- `https://dl.espressif.com/dl/package_esp32_index.json`
- `https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json`

## Key pin assignments

| Signal             | Chip  | GPIO |
|--------------------|-------|------|
| Green LED          | RP2040 | 25  |
| Blue LED           | ESP32  | 32  |
| Yellow LED         | ESP32  | 33  |
| UART TX → RP2040   | ESP32  | 19  |
| UART RX ← RP2040   | ESP32  | 22  |
| UART TX → ESP32    | RP2040 | 1   |
| UART RX ← ESP32    | RP2040 | 0   |
| I2S microphone data | ESP32 | 25  |
| I2S microphone CLK  | ESP32 | 27  |
| IMU I2C SDA        | ESP32  | 18  |
| IMU I2C SCL        | ESP32  | 21  |
| RP2040 reset       | ESP32  | 23  |

See `REFERENCE.md` for the full UEXT and header pinouts.

## Coding conventions

- Each sketch lives in its own subdirectory named after the sketch (Arduino IDE requirement)
- Each sketch directory should include a `README.md` describing purpose, required libraries, board selection, and wiring if applicable
- Serial baud rate: use **115200** unless a sensor or protocol requires otherwise
