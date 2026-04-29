# SmartHomeESP32

Wi-Fi + MQTT hub that bridges the Udoo Key to a local smart home broker. It does two things simultaneously:

1. Receives DHT22 (or BME280) sensor readings from the RP2040 over the on-board UART and publishes them to MQTT every 10 seconds.
2. Reads the on-board I2S microphone and detects sound events — knocking, glass breaking, a doorbell — publishing each event to MQTT. **(Udoo Key Pro only for audio.)**

Must be used together with [`rp2040/SmartHomeRP2040/`](../../rp2040/SmartHomeRP2040/).

## Before flashing

Edit `config.h` and fill in your Wi-Fi credentials and local MQTT broker address. Consider adding `config.h` to `.gitignore` to avoid committing credentials.

## Board selection

In Arduino IDE select **ESP32 Dev Module**.

## Required libraries

Install via Arduino IDE Library Manager:
- **PubSubClient** by Nick O'Leary

Built-in (no install needed):
- `WiFi.h` — ESP32 Arduino core
- `I2S.h` — ESP32 Arduino core

## Pins used

| ESP32 pin | Function |
|-----------|----------|
| GPIO22 | UART RX ← RP2040 GPIO1 |
| GPIO19 | UART TX → RP2040 GPIO0 (unused) |
| GPIO25 | I2S microphone data (Pro only) |
| GPIO27 | I2S microphone CLK (Pro only) |
| GPIO32 | Blue on-board LED (blinks on sensor publish) |
| GPIO33 | Yellow on-board LED (flashes on audio event) |

## MQTT topics

| Topic | Payload | Notes |
|-------|---------|-------|
| `udookey/sensor/temperature` | float, e.g. `23.40` | Published every 10 s |
| `udookey/sensor/humidity` | float, e.g. `56.70` | Published every 10 s |
| `udookey/audio/event` | `knock`, `glass_break`, `doorbell`, `loud_event` | Published on detection |
| `udookey/status` | `online` | Retained, published on connect |

## Serial output

Open the Serial Monitor at **115200 baud**.

```
[WiFi] Connecting.... OK
[MQTT] Connecting OK
[AUDIO] I2S microphone ready
SmartHomeESP32 ready
[SENSOR] T=22.50 H=58.20 published
[AUDIO] Event: knock (energy=1.23e+09 ZCR=0.082)
```

On a standard Udoo Key (no microphone), the I2S line will read:
```
[AUDIO] I2S init failed — audio detection disabled (Udoo Key Pro required)
```
Sensor MQTT continues to work normally in that case.

## Tuning the audio thresholds

The sketch classifies sounds using two features computed per 32 ms frame at 16 kHz:

- **Energy** — mean-square amplitude. Silent frames are ignored below `ENERGY_THRESHOLD`.
- **ZCR** (Zero Crossing Rate per sample) — high for high-frequency sounds (glass breaking), low for low-frequency impulses (knocking, doorbell tones).

| ZCR band | Classification |
|----------|---------------|
| ZCR < 0.15 | knock / doorbell (low-freq impulse; 2–5 in 1.5 s → knock, more → doorbell) |
| 0.15 ≤ ZCR < 0.30 | loud_event |
| ZCR ≥ 0.30 | glass_break |

To tune, open the Serial Monitor and intentionally make each sound type near the board. Read the `energy=` and `ZCR=` values printed for each event, then adjust `ENERGY_THRESHOLD`, `ZCR_KNOCK_MAX`, and `ZCR_GLASS_MIN` in the sketch so each class falls cleanly in the expected band.

Note: doorbell detection is an approximation — the sketch counts rapid low-ZCR impulses rather than analyzing frequency. It works for most chimes but may need tuning for unusual doorbells.
