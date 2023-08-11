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


## On-board LED Pins

|Led|Chip|Pin|
|---|---|---|
|Green |RP2040|Gpio25|
|Yellow|ESP32 |Gpio32|
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
