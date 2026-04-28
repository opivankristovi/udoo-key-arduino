# dht22_data_plot

Reads temperature and humidity from a DHT22 sensor connected to GPIO22 and prints the values (including computed heat index) to the Serial Monitor.

## Required libraries

Install these through the Arduino IDE Library Manager:

- **DHT sensor library** by Adafruit
- **Adafruit Unified Sensor** by Adafruit

## Board selection

In Arduino IDE select **Raspberry Pi Pico** as the target board.

## Wiring

Connect the DHT22 to a 3.3 V supply and to **GPIO22** for data. Most DHT22 breakout boards include the required pull-up resistor; if using a bare sensor add a 10 kΩ pull-up between the data line and 3.3 V.

| DHT22 pin | RP2040 pin |
|-----------|-----------|
| VCC       | 3.3 V     |
| GND       | GND       |
| DATA      | GPIO22    |

## Serial output

Open the Serial Monitor at **9600 baud**.

Example output:
```
Humidity: 52.30%  Temperature: 22.60°C 72.68°F  Heat index: 22.16°C 71.89°F
```
