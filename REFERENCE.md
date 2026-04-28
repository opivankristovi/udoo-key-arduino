# Quick Reference

## Chip to Chip Connections

#### Serial Connection
|Chip|Pin|Function|
|---|---|---|
|ESP32 |Gpio19|Uart Tx to rp2040|
|ESP32 |Gpio22|Uart Rx from rp2040|
|RP2040|Gpio1 |Uart Tx to esp32|
|RP2040|Gpio0 |Uart Rx from esp32|

#### SWD Connection
|ESP32|RP2040|Function|
|---|---|---|
|Gpio2|SWDIO|rp2040 SWD|
|Gpio4|SWCLK|rp2040 SWD|
|Gpio5| n/a |Set high to use, Set low to use external SWD|

|ESP32|RP2040|Function|
|---|---|---|
|Gpio23|Reset pin|Reset rp2040 by setting low then setting high|

## MPU-6500 IMU (Udoo Key Pro only)

The MPU-6500 6-axis IMU (accelerometer + gyroscope) is physically connected to **both** MCUs, but a jumper selects which one may communicate with it over I2C. Pin 1 is closest to the RP2040; pin 3 is closest to the ESP32.

| Jumper position | IMU connected to     |
|-----------------|----------------------|
| Not placed      | ESP32 **(default)**  |
| Pin 1–2         | RP2040               |
| Pin 2–3         | ESP32                |

### I2C pins when using ESP32
| ESP32  | Function |
|--------|----------|
| Gpio18 | I2C SDA  |
| Gpio21 | I2C SCL  |

I2C address: **0x69**

### I2C pins when using RP2040
Verify the specific SDA/SCL GPIOs from the board schematic before use.

## I2S pins for SPK0838HT4H-1 digital microphone
|ESP32|Microphone sensor|Function|
|---|---|---|
|Gpio25|I2S DO|I2S data pin|
|Gpio27|I2S CLK|I2S clock pin|

## On-board LED Pins

|Led|Chip|Pin|
|---|---|---|
|Green |RP2040|Gpio25|
|Yellow|ESP32 |Gpio33|
|Blue  |ESP32 |Gpio32|

## UEXT Pins
|               |               |
| ---           | ---           |
| CS   (Gpio15) | SCK  (Gpio14) |
| MOSI (Gpio12) | MISO (Gpio35) |
| SDA  (Gpio18) | SCL  (Gpio21) |
| RXD  (Gpio26) | TXD  (Gpio13) |
| GND           | 3.3V          |

![](http://www.udoo.org/docs-key/img/p3_header.jpg)

## Rp2040 Pinout

![](http://www.udoo.org/docs-key/img/p1_p2_p5_headers.png)
