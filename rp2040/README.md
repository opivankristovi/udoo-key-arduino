Examples in this folder are to be flashed to the RP2040 controller.

### Note:

For more detailed information about each sketch, take a look at the commented sections in the code.

For a cheatsheet with the pinout and onboard connections etc. can be found here: [Reference](https://github.com/opivankristovi/udoo-key-arduino/blob/main/REFERENCE.md)

## For all Udoo Key boards
[Blink onboard LED](https://github.com/opivankristovi/udoo-key-arduino/tree/main/rp2040/BlinkRP2040)
  The "Hello World" code for microcontrollers; blinking the onboard LED on and off.
  
[Serial UART connection with ESP32](https://github.com/opivankristovi/udoo-key-arduino/tree/main/rp2040/picoToEsp32UART)
  Example of how to read/write serial UART commands to/from ESP32

[Readout of the rp2040's internal temperaturesensor](https://github.com/opivankristovi/udoo-key-arduino/tree/main/rp2040/core_temperature_plotter)
  A simple example of how to get data from the rp2040's internal thermometer and write to serial monitor.

[DHT22_data_plotter](https://github.com/opivankristovi/udoo-key-arduino/tree/main/rp2040/dht22_data_plot)
  Get the sensor values from a DHT22-sensor connected to GPIOpin 22 and write it to the serial monitor. 

## For Udoo Key Pro only

The MPU-6500 IMU can be accessed from the RP2040 by setting the on-board jumper to **pin 1–2** (pin 1 is the pin closest to the RP2040). By default (no jumper) the IMU is connected to the ESP32.

No RP2040 IMU example exists yet — contributions welcome. See [REFERENCE.md](https://github.com/opivankristovi/udoo-key-arduino/blob/main/REFERENCE.md) for the jumper details, and use the `/uart-pair` Claude Code command if you want to scaffold a sketch that reads the IMU on the RP2040 side.
