The sketches in this folder are for writing to the ESP32 controller
## For Udoo Key Pro only:
[esp32/MPU6500_Test](https://github.com/opivankristovi/udoo-key-arduino/tree/main/esp32/MPU6500_Test_esp32)
  Reads accelerometer, gyroscope, and temperature from the on-board MPU-6500 IMU.
  Requires the IMU jumper to be not placed (default) or set to pin 2–3 so the ESP32 controls the I2C bus.

[read samples from I2S microphone](https://github.com/opivankristovi/udoo-key-arduino/tree/main/esp32/Digital_mic_SerialPlotter)
  Gives a stream of sample values to the monitor, read from the on board I2S microphone sensor

## For any Udoo Key
[Blink onboard LEDs](https://github.com/opivankristovi/udoo-key-arduino/tree/main/esp32/BlinkESP32)
  Alternates the blue and yellow on-board LEDs. Good first sketch to verify the ESP32 toolchain is set up correctly.

[WiFi Scanner](https://github.com/opivankristovi/udoo-key-arduino/tree/main/esp32/WiFiScan)
  Scans for nearby Wi-Fi networks and prints each network's SSID, signal strength, and security type to the Serial Monitor. No credentials required.

[Serial UART >> RP2040](https://github.com/opivankristovi/udoo-key-arduino/tree/main/esp32/esp32ToPicoUART)
  Should be used together with the RP2040 UART code to work.
  Simultaneously with blinking its onboard LED, it sends a command through the onboard UART0 bus to the RP2040 which in turn uses this command to blink its own LED in sync.
