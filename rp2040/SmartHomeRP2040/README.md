# SmartHomeRP2040

Reads temperature and humidity from a DHT22 sensor connected to GPIO22 and transmits the values over the on-board UART to the ESP32 every 10 seconds.

Must be used together with [`esp32/SmartHomeESP32/`](../../esp32/SmartHomeESP32/).

## Board selection

In Arduino IDE select **Raspberry Pi Pico**.

## Required libraries

Install via Arduino IDE Library Manager:
- **DHT sensor library** by Adafruit
- **Adafruit Unified Sensor** by Adafruit

If switching to BME280 (see below):
- **Adafruit BME280 Library** by Adafruit

## Wiring

| DHT22 pin | RP2040 pin |
|-----------|-----------|
| VCC | 3.3 V |
| GND | GND |
| DATA | GPIO22 |

Most DHT22 breakout boards include the pull-up resistor. If using a bare sensor, add a 10 kΩ resistor between DATA and 3.3 V.

The on-board UART connection to the ESP32 (GPIO0/GPIO1) is internal — no extra wiring required.

## Pins used

| RP2040 pin | Function |
|------------|----------|
| GPIO22 | DHT22 data |
| GPIO1 | UART TX → ESP32 GPIO22 |
| GPIO0 | UART RX ← ESP32 GPIO19 (unused) |

## Serial output

Open the Serial Monitor at **115200 baud**.

```
SmartHomeRP2040 ready
[UART TX] T:22.50,H:58.20
[UART TX] T:22.60,H:58.10
```

If the DHT22 returns bad data:
```
[SENSOR] Read failed — skipping
```

## Swapping DHT22 for BME280

To use a BME280 over I2C instead of the DHT22:
1. Uncomment `#define USE_BME280` at the top of the sketch.
2. Identify the correct SDA/SCL GPIO pins for the RP2040 from the board schematic before wiring (the Udoo Key schematic is linked in the [Quick Reference](../../REFERENCE.md)).
3. Uncomment the `Wire.begin()` and `bme.begin()` lines in `setup()`, adding the correct SDA/SCL pin numbers if they are not the RP2040 defaults.
4. Uncomment the temperature and humidity reads in `readSensor()`.
5. Install the **Adafruit BME280 Library** via Library Manager.

## Notes

- The DHT22 datasheet specifies a minimum sampling interval of 2 seconds. This sketch reads every 10 seconds, well within spec. Do not lower `READ_INTERVAL_MS` below 2000.
- The UART line format is `T:<temp>,H:<hum>\n` (Celsius, two decimal places). The ESP32 side parses this and publishes the values to MQTT.
