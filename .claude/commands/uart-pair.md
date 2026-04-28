---
description: "Scaffold a matched ESP32 + RP2040 UART communication pair for the Udoo Key"
argument-hint: "<PairName> [brief description]"
allowed-tools: ["Bash", "Read", "Write", "Edit"]
---

# UART Pair — Udoo Key Arduino

Scaffold a matched ESP32 + RP2040 UART sketch pair that communicate over the Udoo Key's on-board UART bus.

**Arguments:** "$ARGUMENTS"

## Background

The Udoo Key has a dedicated on-board UART connection between the two MCUs:

| Signal              | ESP32 GPIO | RP2040 GPIO |
|---------------------|-----------|-------------|
| ESP32 TX → RP2040 RX | GPIO19   | GPIO0       |
| ESP32 RX ← RP2040 TX | GPIO22   | GPIO1       |

Both sides use 9600 baud, 8N1. On the ESP32, `Serial1` maps to UART0. On the RP2040, `Serial1` is used with explicit `setRX`/`setTX` calls.

## Instructions

Parse the arguments:
- **arg 1** — PairName (PascalCase, no spaces): used as the base name; ESP32 sketch will be `<PairName>ESP32`, RP2040 sketch will be `<PairName>RP2040`
- **arg 2+** — optional brief description of what the pair does

If arg 1 is missing, ask the user to provide it before proceeding.

---

## Step 1 — Create the ESP32 sketch

**Directory:** `esp32/<PairName>ESP32/`
**File:** `esp32/<PairName>ESP32/<PairName>ESP32.ino`

```cpp
const int UART_RX    = 22;  // UART RX from RP2040
const int UART_TX    = 19;  // UART TX to RP2040
const int BLUE_LED   = 32;  // Blue on-board LED
const int YELLOW_LED = 33;  // Yellow on-board LED
```

`setup()`:
- `Serial1.begin(9600, SERIAL_8N1, UART_RX, UART_TX)`
- `Serial.begin(115200)` for USB debug monitor
- Print a startup message like `"<PairName>ESP32 ready"`

`loop()`: implement the ESP32 side of whatever the user described (or a sensible default: send a numbered message to the RP2040 every second, print any received reply to the debug monitor).

---

## Step 2 — Create the ESP32 README

**File:** `esp32/<PairName>ESP32/README.md`

- One-paragraph description of the ESP32 side's role
- Note that this sketch must be used together with `rp2040/<PairName>RP2040/`
- **Board selection:** `ESP32 Dev Module`
- **UART pins** table (GPIO19/22)
- Baud rate: 9600 (inter-chip), 115200 (debug USB)

---

## Step 3 — Create the RP2040 sketch

**Directory:** `rp2040/<PairName>RP2040/`
**File:** `rp2040/<PairName>RP2040/<PairName>RP2040.ino`

```cpp
const int UART_RX   = 0;   // UART RX from ESP32
const int UART_TX   = 1;   // UART TX to ESP32
const int GREEN_LED = 25;  // Green on-board LED
```

`setup()`:
- `Serial1.setRX(UART_RX); Serial1.setTX(UART_TX);`
- `Serial1.begin(9600)`
- `Serial.begin(115200)` for USB debug monitor
- Print a startup message like `"<PairName>RP2040 ready"`

`loop()`: implement the RP2040 side that corresponds to the ESP32 side (or a sensible default: read incoming messages from Serial1, print them to the debug monitor, echo a reply back).

---

## Step 4 — Create the RP2040 README

**File:** `rp2040/<PairName>RP2040/README.md`

- One-paragraph description of the RP2040 side's role
- Note that this sketch must be used together with `esp32/<PairName>ESP32/`
- **Board selection:** `Raspberry Pi Pico`
- **UART pins** table (GPIO0/1)
- Baud rate: 9600 (inter-chip), 115200 (debug USB)

---

## Step 5 — Update both parent READMEs

- Append a row to the sketch table in `esp32/README.md` for `<PairName>ESP32`
- Append a row to the sketch table in `rp2040/README.md` for `<PairName>RP2040`

---

## Step 6 — Confirm

Print a short summary: all files created, both README rows added, and a reminder to flash each sketch to the correct MCU before testing.
