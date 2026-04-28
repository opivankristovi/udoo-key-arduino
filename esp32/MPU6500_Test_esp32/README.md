# MPU6500_Test_esp32

Reads accelerometer, gyroscope, and temperature data from the on-board MPU-6500 IMU and prints it to the Serial Monitor. Performs a calibration step on startup (keep the board level during the first ~10 seconds).

**Udoo Key Pro only** — the MPU-6500 is not present on the standard Udoo Key.

## Jumper requirement

The IMU is physically connected to both MCUs. To use it from the ESP32 you must have the jumper **not placed** (default) or set to **pin 2–3**. Pin 1 is closest to the RP2040; pin 3 is closest to the ESP32.

| Jumper position | IMU connected to |
|-----------------|-----------------|
| Not placed      | ESP32 (default) |
| Pin 2–3         | ESP32           |
| Pin 1–2         | RP2040 ← wrong for this sketch |

## Required library

Install via Arduino IDE Library Manager:

- **FastIMU** by LiquidCGS

## Board selection

In Arduino IDE select **ESP32 Dev Module**.

## Pins used

| ESP32 pin | Function |
|-----------|----------|
| GPIO18    | I2C SDA  |
| GPIO21    | I2C SCL  |

I2C address: **0x69**

## Serial output

Open the Serial Monitor at **115200 baud**.

During calibration the sketch prints the computed biases, then streams live readings:

```
Accel - x:0.01   y:-0.02   z:1.00   Gyro - x:0.12   y:-0.05   z:0.03   Temp:28.4
```
