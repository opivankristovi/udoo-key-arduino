# RepeaterNodeRP2040

Sensor / IO node — the RP2040 half of the Udoo Key Repeater Node pair.

Must be used together with **[esp32/RepeaterNodeESP32](../../esp32/RepeaterNodeESP32/)**.

## What it does

- Reads up to two I2C sensors (BME280 / BMP280 / BMP180, selectable per device)
- Reads a temperature/humidity probe (DS18B20 1-wire or DHT11/DHT22 single-wire)
- Reads two analog inputs (12-bit ADC, 0-4095) and two digital inputs
- Sends all readings as a JSON packet to the ESP32 every 10 seconds
- Receives output commands from the ESP32 and drives four digital/PWM pins

## Board & toolchain

- **Board:** Raspberry Pi Pico (RP2040)
- **Arduino IDE** with the arduino-pico board package (Earle Philhower)
- Board manager URL: `https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json`

## Required libraries

Install only what your sensor configuration needs:

| Library | Required for |
|---------|-------------|
| Adafruit BME280 Library + Adafruit Unified Sensor | `I2C_BME280` |
| Adafruit BMP280 Library + Adafruit Unified Sensor | `I2C_BMP280` |
| Adafruit BMP085 Library + Adafruit Unified Sensor | `I2C_BMP180` |
| DHT sensor library (Adafruit) + Adafruit Unified Sensor | `PROBE_DHT11` / `PROBE_DHT22` |
| OneWire + DallasTemperature | `PROBE_DS18B20` |
| **ArduinoJson v7** (v6 will not compile) | always required |

## Configuration

Edit the **CONFIGURATION** block near the top of the `.ino`:

```cpp
#define I2C_A_TYPE  I2C_BME280   // type of first I2C sensor
#define I2C_A_ADDR  0x76
#define I2C_B_TYPE  I2C_NONE     // I2C_NONE to disable second sensor
#define I2C_B_ADDR  0x77
#define PROBE_TYPE  PROBE_DHT22  // PROBE_NONE to disable
#define READ_INTERVAL_MS  10000UL
```

Available type values: `I2C_NONE`, `I2C_BME280`, `I2C_BMP280`, `I2C_BMP180`, `PROBE_NONE`, `PROBE_DS18B20`, `PROBE_DHT11`, `PROBE_DHT22`.

## Default pin map

| Channel | GPIO | Notes |
|---------|------|-------|
| I2C SDA | GP4 | BME280 / BMP280 / BMP180 |
| I2C SCL | GP5 | |
| Probe | GP16 | DS18B20 or DHT data pin; 4.7 kΩ pull-up to 3V3 |
| Analog input 1 | GP26 | 12-bit ADC (0-4095) |
| Analog input 2 | GP27 | 12-bit ADC |
| Digital input 1 | GP14 | Active-low with internal pull-up |
| Digital input 2 | GP15 | Active-low with internal pull-up |
| Output 1 | GP17 | Digital or PWM (0-255) |
| Output 2 | GP18 | |
| Output 3 | GP19 | |
| Output 4 | GP20 | |
| UART RX (from ESP32) | GP0 | On-board bridge |
| UART TX (to ESP32) | GP1 | On-board bridge |
| Green LED | GP25 | On-board |

> Only GP26, GP27, GP28 are ADC-capable on the RP2040. Do not use other pins for analog inputs.

## UART protocol

**9600 baud** on `Serial1` (GP0/GP1). Lines are newline-terminated.

Sensor packet sent every `READ_INTERVAL_MS`:
```
S:{"ia":{"t":23.40,"h":65.20,"p":1013.25},"pr":{"t":22.80,"h":70.10},"an":[512,1024],"di":[0,1]}\n
```

Fields: `ia` = I2C device A, `ib` = I2C device B, `pr` = probe, `an` = analog array, `di` = digital inputs.
Keys within a device object: `t` = temperature (°C), `h` = humidity (%), `p` = pressure (hPa).
Missing keys mean the field is not available for the selected sensor type.

Output command received from ESP32:
```
C:{"o":0,"v":255}\n
```
`o` = output index (0-based, 0-3), `v` = value (0-255; 0 = off, 255 = full on, 1-254 = PWM).
