# udoo-key-arduino
Provides a few simple usage examples of the Udoo Key (Pro) and its components using Arduino IDE, to get started. There isn't much information or examples available about this board yet, so I tought I'd better share it here while I'm figuring it out myself, so others can either benefit from or contribute to it.

## Description

The Udoo Key is a development board with an ESP32 and an RP2040.

This repository has some Arduino code for the Udoo Key that should run as is. Feel 
free to take a look. There is also an Udoo Key Pro that comes with a microphone
and an IMU, I'll try to provide examples for those as well, but by all means: feel free to contribute if you think you can add or improve something.

Read more about the Udoo Key:
* [Kickstarter](https://www.kickstarter.com/projects/udoo/udoo-key-the-4-ai-platform)
* [Udoo Key Docs](http://www.udoo.org/docs-key/Introduction/Introduction.html)

## Preparation
Before you can program the Udoo Key, you need to have installed additional boards in the Arduino IDE for both its mcu's.
To do this, go to File>>Preferences and under "Additional Board Manager URL's" add the following 2 URL's :
* https://dl.espressif.com/dl/package_esp32_index.json
* https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
When this is done, you can go to the Board Manager to search for and install the Raspberry Pi RP2040 and Espressif ESP32 boards.
For compiling or programming the RP2040, select the Raspberry Pi Pico Module, and for the ESP32, select the ESP32 Dev Module.


I've collected some pinouts etc from the Udoo Key documentation and 
put them in a [Quick Reference](REFERENCE.md) to save time.

## Using this repo with Claude Code

This repository includes [Claude Code](https://claude.ai/code) integration. If you open it in Claude Code, the `CLAUDE.md` file gives Claude full context about the board — pin assignments, board selection, jumper positions, and coding conventions — so you can ask questions or request code without explaining the hardware first.

Two slash commands are available to scaffold new sketches quickly:

### `/new-sketch <esp32|rp2040> <SketchName> [description]`

Creates a new sketch directory with a ready-to-compile `.ino` (correct board-specific pin constants, `setup()`/`loop()` stubs) and a `README.md`, and adds it to the relevant index.

```
/new-sketch rp2040 I2CScanner Scans the I2C bus and prints detected addresses
/new-sketch esp32 WebServer Serves a simple status page over Wi-Fi
```

### `/uart-pair <PairName> [description]`

Scaffolds a matched ESP32 + RP2040 UART pair in one go — both `.ino` files pre-wired to the on-board UART bus, with READMEs that reference each other.

```
/uart-pair PingPong ESP32 sends a ping every second, RP2040 echoes it back
```

---

## [esp32](esp32/README.md)

Programs that are meant to be run on the ESP32 are in the [esp32](esp32/) directory.

| Sketch | Description |
|--------|-------------|
| [BlinkESP32](esp32/BlinkESP32/) | Alternates the blue and yellow on-board LEDs |
| [WiFiScan](esp32/WiFiScan/) | Scans for nearby Wi-Fi networks (no credentials needed) |
| [esp32ToPicoUART](esp32/esp32ToPicoUART/) | Sends UART commands to the RP2040 |
| [Digital_mic_SerialPlotter](esp32/Digital_mic_SerialPlotter/) | Streams I2S microphone samples *(Pro only)* |
| [MPU6500_Test_esp32](esp32/MPU6500_Test_esp32/) | Reads accelerometer and gyroscope data *(Pro only)* |
| [SmartHomeESP32](esp32/SmartHomeESP32/) | Wi-Fi + MQTT hub: UART sensor receiver + I2S audio event detection *(Pro only for audio)* |

## [rp2040](rp2040/README.md)

Programs that are meant to be run on the RP2040 are in the [rp2040](rp2040/) directory.

| Sketch | Description |
|--------|-------------|
| [BlinkRP2040](rp2040/BlinkRP2040/) | Blinks the green on-board LED |
| [core_temperature_plotter](rp2040/core_temperature_plotter/) | Reads the RP2040 internal temperature sensor |
| [dht22_data_plot](rp2040/dht22_data_plot/) | Reads a DHT22 temperature/humidity sensor |
| [picoToEsp32UART](rp2040/picoToEsp32UART/) | Receives UART commands from the ESP32 |
| [SmartHomeRP2040](rp2040/SmartHomeRP2040/) | Reads DHT22 and transmits sensor data to ESP32 over UART |
