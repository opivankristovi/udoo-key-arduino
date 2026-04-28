---
description: "Scaffold a new Arduino sketch for the Udoo Key board"
argument-hint: "<esp32|rp2040> <SketchName> [brief description]"
allowed-tools: ["Bash", "Read", "Write", "Edit"]
---

# New Sketch — Udoo Key Arduino

Scaffold a new Arduino sketch for the Udoo Key board.

**Arguments:** "$ARGUMENTS"

## Instructions

Parse the arguments:
- **arg 1** — target MCU: `esp32` or `rp2040`
- **arg 2** — sketch name (PascalCase, no spaces — Arduino IDE requires the directory name to match the sketch name exactly)
- **arg 3+** — optional brief description (used in the README and parent README table)

If arg 1 or arg 2 is missing, ask the user to provide them before proceeding.

---

## Step 1 — Create the sketch directory and `.ino` file

**Directory:** `<mcu>/<SketchName>/`
**File:** `<mcu>/<SketchName>/<SketchName>.ino`

### For ESP32 sketches

Use these on-board pin constants at the top of the file:

```cpp
const int BLUE_LED   = 32;  // Blue on-board LED
const int YELLOW_LED = 33;  // Yellow on-board LED
const int UART_RX    = 22;  // UART RX from RP2040
const int UART_TX    = 19;  // UART TX to RP2040
```

Only include the constants that the sketch actually uses. Add a `setup()` with `Serial.begin(115200)` and a startup message, and a minimal `loop()`. If the sketch description implies Wi-Fi, include `#include <WiFi.h>`. If it implies I2C, include `#include <Wire.h>` and `Wire.begin(18, 21)` (SDA=GPIO18, SCL=GPIO21). Otherwise keep includes minimal.

### For RP2040 sketches

Use these on-board pin constants at the top of the file:

```cpp
const int GREEN_LED  = 25;  // Green on-board LED
const int UART_RX    = 0;   // UART RX from ESP32
const int UART_TX    = 1;   // UART TX to ESP32
```

Only include the constants the sketch actually uses. Add a `setup()` with `Serial.begin(115200)` and a startup message, and a minimal `loop()`. If the description implies I2C, the RP2040 standard I2C pins on the Udoo Key are not fixed to specific GPIOs — leave a comment to check the pinout.

---

## Step 2 — Create `README.md`

**File:** `<mcu>/<SketchName>/README.md`

Include:
- A one-paragraph description of what the sketch does (use the user's description if provided)
- **Board selection** section: `ESP32 Dev Module` for esp32, `Raspberry Pi Pico` for rp2040
- **Serial output** section: baud rate and example output if relevant
- **Pins used** table: only list pins the sketch actually declares/uses
- **Required libraries** section: only if the sketch needs libraries beyond the core (omit if none)

---

## Step 3 — Update the parent README table

Read `<mcu>/README.md`. Find the existing sketch table (markdown table with `| Sketch | Description |` header) and append a new row:

```
| [SketchName](<mcu>/SketchName/) | <description> |
```

Use the user's description or a short generated one if none was given. If no table exists, add one.

---

## Step 4 — Confirm

Print a short summary: the files created and the README row added.
