The sketches in this folder are for writing to the ESP32 controller
## For Udoo Key Pro only:
[esp32/MPU6500_Test](https://github.com/opivankristovi/udoo-key-arduino/tree/main/esp32/MPU6500_Test_esp32)
  Gives an example of retrieving sensor values from the Kickstarters Udoo Key Pro on-board MPU6500 IMU sensor
  printing them to the serial monitor

[read samples from I2S microphone](https://github.com/opivankristovi/udoo-key-arduino/tree/main/esp32/Digital_mic_SerialPlotter)
  Gives a stream of sample values to the monitor, read from the on board I2S microphone sensor

## For any Udoo Key
[Serial UART >> RP2040](https://github.com/opivankristovi/udoo-key-arduino/tree/main/esp32/esp32ToPicoUART)
  Should be used together with the RP2040 UART code to work.
  simultaneously with blinking its onboard LED, it send a command trough the onboard UART0 bus to the RP2040 which on its turn       uses this command to blink its own LED in sync.
