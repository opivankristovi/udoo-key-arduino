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

#Preparation
Before you can program the Udoo Key, you need to have installed additional boards in the Arduino IDE for both its mcu's.
To do this, go to File>>Preferences and under "Additional Board Manager URL's" add the following 2 URL's :
* https://dl.espressif.com/dl/package_esp32_index.json
* https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
When this is done, you can go to the Board Manager to search for and install the Raspberry Pi RP2040 and Espressif ESP32 boards.
For compiling or programming the RP2040, select the Raspberry Pi Pico Module, and for the ESP32, select the ESP32 Dev Module.


I've collected some pinouts etc from the Udoo Key documentation and 
put them in a [Quick Reference](REFERENCE.md) to save time.

## [esp32](esp32/README.md)

Programs that are meant to be run on the esp32 are in the [esp32](esp32/) directory.

## [rp2040](rp2040/README.md)

Programs that are meant to be run on the rp2040 are in the [rp2040](rp2040/) directory.
