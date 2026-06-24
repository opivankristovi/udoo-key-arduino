/*
 * RepeaterNodeRP2040 — sensor / IO node, Udoo Key RP2040 side.
 *
 * Reads two I2C sensors, a temperature/humidity probe (DS18B20 or DHT),
 * two analog inputs, and two digital inputs, then sends a JSON packet to
 * the ESP32 over the on-board UART bridge every READ_INTERVAL_MS ms.
 * Receives output commands from the ESP32 and drives four digital/PWM pins.
 *
 * Counterpart sketch: esp32/RepeaterNodeESP32
 *
 * Board:     Raspberry Pi Pico (RP2040)  — select in Arduino IDE
 * Monitor:   115200 baud (USB Serial)
 * Bridge:    9600 baud (Serial1, GP0/GP1 → ESP32)
 *
 * UART protocol (newline-terminated lines):
 *   RP2040 → ESP32:  S:{json}\n   — sensor packet, sent every READ_INTERVAL_MS
 *   ESP32  → RP2040: C:{json}\n   — output command: {"o":<0-3>,"v":<0-255>}
 *
 * Install only the libraries you need for your sensor configuration:
 *   Adafruit BME280 Library + Adafruit Unified Sensor   — for I2C_BME280
 *   Adafruit BMP280 Library + Adafruit Unified Sensor   — for I2C_BMP280
 *   Adafruit BMP085 Library + Adafruit Unified Sensor   — for I2C_BMP180
 *   DHT sensor library (Adafruit) + Adafruit Unified Sensor  — for DHT11/DHT22
 *   OneWire + DallasTemperature                          — for DS18B20
 *   ArduinoJson v7
 */

// =====================================================================
// SENSOR TYPE CONSTANTS — do not change these values
// =====================================================================
#define I2C_NONE    0
#define I2C_BME280  1   // temperature + humidity + pressure
#define I2C_BMP280  2   // temperature + pressure
#define I2C_BMP180  3   // temperature + pressure

#define PROBE_NONE    0
#define PROBE_DS18B20 1  // 1-wire, temperature only
#define PROBE_DHT11   2  // single-wire, temperature + humidity
#define PROBE_DHT22   3  // single-wire, temperature + humidity (higher accuracy)

// =====================================================================
// CONFIGURATION — edit before flashing
// =====================================================================

// I2C sensor A  (addr 0x76 or 0x77; two different types can share the bus)
#define I2C_A_TYPE  I2C_BME280   // I2C_NONE | I2C_BME280 | I2C_BMP280 | I2C_BMP180
#define I2C_A_ADDR  0x76

// I2C sensor B  (set I2C_NONE to disable)
#define I2C_B_TYPE  I2C_NONE     // I2C_NONE | I2C_BME280 | I2C_BMP280 | I2C_BMP180
#define I2C_B_ADDR  0x77

// Probe sensor on PIN_PROBE (set PROBE_NONE to disable)
#define PROBE_TYPE  PROBE_DHT22  // PROBE_NONE | PROBE_DS18B20 | PROBE_DHT11 | PROBE_DHT22

// How often to read all sensors and send a packet (milliseconds)
#define READ_INTERVAL_MS  10000UL

// =====================================================================
// PIN MAP — adjust to match your wiring
// =====================================================================
static const int PIN_I2C_SDA = 4;
static const int PIN_I2C_SCL = 5;
static const int PIN_PROBE   = 16;           // DS18B20 data or DHT data pin
static const int PIN_AIN[2]  = {26, 27};    // ADC-capable: GP26 / GP27 / GP28 only
static const int PIN_DIN[2]  = {14, 15};    // digital inputs (active-low with pull-up)
static const int PIN_OUT[4]  = {17, 18, 19, 20}; // digital / PWM outputs
static const int LED_GREEN   = 25;           // on-board green LED

// UART bridge to ESP32
static const int UART_RX_PIN = 0;   // GP0 — connected to ESP32 GPIO19 (TX)
static const int UART_TX_PIN = 1;   // GP1 — connected to ESP32 GPIO22 (RX)
static const int UART_BAUD   = 9600;

// =====================================================================
// CONDITIONAL LIBRARY INCLUDES
// =====================================================================
#include <Wire.h>
#include <ArduinoJson.h>

#if (I2C_A_TYPE == I2C_BME280) || (I2C_B_TYPE == I2C_BME280)
  #include <Adafruit_BME280.h>
  static Adafruit_BME280 bme[2];      // [0] = device A, [1] = device B
  static bool            bmeOk[2] = {};
#endif

#if (I2C_A_TYPE == I2C_BMP280) || (I2C_B_TYPE == I2C_BMP280)
  #include <Adafruit_BMP280.h>
  static Adafruit_BMP280 bmp280[2];
  static bool            bmp280Ok[2] = {};
#endif

#if (I2C_A_TYPE == I2C_BMP180) || (I2C_B_TYPE == I2C_BMP180)
  #include <Adafruit_BMP085.h>        // covers the register-compatible BMP180
  static Adafruit_BMP085 bmp180[2];
  static bool            bmp180Ok[2] = {};
#endif

#if PROBE_TYPE == PROBE_DS18B20
  #include <OneWire.h>
  #include <DallasTemperature.h>
  static OneWire           oneWire(PIN_PROBE);
  static DallasTemperature ds(&oneWire);
#endif

#if (PROBE_TYPE == PROBE_DHT11) || (PROBE_TYPE == PROBE_DHT22)
  #include <DHT.h>
  static DHT dht(PIN_PROBE, PROBE_TYPE == PROBE_DHT11 ? DHT11 : DHT22);
#endif

// =====================================================================
// GLOBALS
// =====================================================================
static unsigned long lastReadMs = 0;
static String        rxBuf;
static bool          rxOverflow = false;

// =====================================================================
// HELPERS
// =====================================================================

// Round to 2 decimal places (keeps JSON compact)
static float r2(float v) { return roundf(v * 100.0f) / 100.0f; }

// =====================================================================
// SENSOR READING + PACKET BUILD
// =====================================================================
static void sendSensorPacket() {
    JsonDocument doc;

    // --- I2C device A ---
#if I2C_A_TYPE == I2C_BME280
    if (bmeOk[0]) {
        doc["ia"]["t"] = r2(bme[0].readTemperature());
        doc["ia"]["h"] = r2(bme[0].readHumidity());
        doc["ia"]["p"] = r2(bme[0].readPressure() / 100.0f);
    }
#elif I2C_A_TYPE == I2C_BMP280
    if (bmp280Ok[0]) {
        doc["ia"]["t"] = r2(bmp280[0].readTemperature());
        doc["ia"]["p"] = r2(bmp280[0].readPressure() / 100.0f);
    }
#elif I2C_A_TYPE == I2C_BMP180
    if (bmp180Ok[0]) {
        doc["ia"]["t"] = r2(bmp180[0].readTemperature());
        doc["ia"]["p"] = r2((float)bmp180[0].readPressure() / 100.0f);
    }
#endif

    // --- I2C device B ---
#if I2C_B_TYPE == I2C_BME280
    if (bmeOk[1]) {
        doc["ib"]["t"] = r2(bme[1].readTemperature());
        doc["ib"]["h"] = r2(bme[1].readHumidity());
        doc["ib"]["p"] = r2(bme[1].readPressure() / 100.0f);
    }
#elif I2C_B_TYPE == I2C_BMP280
    if (bmp280Ok[1]) {
        doc["ib"]["t"] = r2(bmp280[1].readTemperature());
        doc["ib"]["p"] = r2(bmp280[1].readPressure() / 100.0f);
    }
#elif I2C_B_TYPE == I2C_BMP180
    if (bmp180Ok[1]) {
        doc["ib"]["t"] = r2(bmp180[1].readTemperature());
        doc["ib"]["p"] = r2((float)bmp180[1].readPressure() / 100.0f);
    }
#endif

    // --- Probe ---
#if PROBE_TYPE == PROBE_DS18B20
    ds.requestTemperatures();
    float dsT = ds.getTempCByIndex(0);
    if (dsT != DEVICE_DISCONNECTED_C) doc["pr"]["t"] = r2(dsT);
#elif (PROBE_TYPE == PROBE_DHT11) || (PROBE_TYPE == PROBE_DHT22)
    float dhtT = dht.readTemperature();
    float dhtH = dht.readHumidity();
    if (!isnan(dhtT)) doc["pr"]["t"] = r2(dhtT);
    if (!isnan(dhtH)) doc["pr"]["h"] = r2(dhtH);
#endif

    // --- Analog inputs (12-bit: 0–4095) ---
    JsonArray an = doc["an"].to<JsonArray>();
    for (int pin : PIN_AIN) an.add(analogRead(pin));

    // --- Digital inputs (1 = pressed; active-low with pull-up) ---
    JsonArray di = doc["di"].to<JsonArray>();
    for (int pin : PIN_DIN) di.add(digitalRead(pin) == LOW ? 1 : 0);

    // --- Transmit ---
    Serial1.print("S:");
    serializeJson(doc, Serial1);
    Serial1.print('\n');

    Serial.print("[TX] S:");
    serializeJson(doc, Serial);
    Serial.println();

    // Blink to signal transmission
    digitalWrite(LED_GREEN, HIGH); delay(80); digitalWrite(LED_GREEN, LOW);
}

// =====================================================================
// OUTPUT COMMAND RECEIVE
// =====================================================================
static void applyCommand(const String& json) {
    JsonDocument doc;
    if (deserializeJson(doc, json)) return;
    int o = doc["o"] | -1;
    int v = constrain((int)(doc["v"] | 0), 0, 255);
    if (o < 0 || o > 3) return;
    analogWrite(PIN_OUT[o], v);
    Serial.printf("[CMD] output%d = %d\n", o + 1, v);
}

static void receiveCommands() {
    while (Serial1.available()) {
        char c = (char)Serial1.read();
        if (c == '\n' || c == '\r') {
            if (c == '\n' && !rxOverflow && rxBuf.length() > 2 && rxBuf.startsWith("C:"))
                applyCommand(rxBuf.substring(2));
            rxBuf      = "";
            rxOverflow = false;
        } else {
            rxBuf += c;
            if (rxBuf.length() > 128) { rxBuf = ""; rxOverflow = true; }
        }
    }
}

// =====================================================================
// SETUP
// =====================================================================
void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n[RepeaterNodeRP2040] starting");

    pinMode(LED_GREEN, OUTPUT);
    digitalWrite(LED_GREEN, LOW);

    // UART bridge to ESP32
    Serial1.setRX(UART_RX_PIN);
    Serial1.setTX(UART_TX_PIN);
    Serial1.begin(UART_BAUD);

    // 12-bit ADC for better analog precision (0–4095)
    analogReadResolution(12);

    // I2C bus
    Wire.setSDA(PIN_I2C_SDA);
    Wire.setSCL(PIN_I2C_SCL);
    Wire.begin();

    // --- Init I2C sensor A ---
#if I2C_A_TYPE == I2C_BME280
    bmeOk[0] = bme[0].begin(I2C_A_ADDR, &Wire);
    Serial.printf("[I2C-A] BME280 @ 0x%02X: %s\n", I2C_A_ADDR, bmeOk[0] ? "OK" : "FAIL");
#elif I2C_A_TYPE == I2C_BMP280
    bmp280Ok[0] = bmp280[0].begin(I2C_A_ADDR, &Wire);
    Serial.printf("[I2C-A] BMP280 @ 0x%02X: %s\n", I2C_A_ADDR, bmp280Ok[0] ? "OK" : "FAIL");
#elif I2C_A_TYPE == I2C_BMP180
    bmp180Ok[0] = bmp180[0].begin();
    Serial.printf("[I2C-A] BMP180: %s\n", bmp180Ok[0] ? "OK" : "FAIL");
#endif

    // --- Init I2C sensor B ---
#if I2C_B_TYPE == I2C_BME280
    bmeOk[1] = bme[1].begin(I2C_B_ADDR, &Wire);
    Serial.printf("[I2C-B] BME280 @ 0x%02X: %s\n", I2C_B_ADDR, bmeOk[1] ? "OK" : "FAIL");
#elif I2C_B_TYPE == I2C_BMP280
    bmp280Ok[1] = bmp280[1].begin(I2C_B_ADDR, &Wire);
    Serial.printf("[I2C-B] BMP280 @ 0x%02X: %s\n", I2C_B_ADDR, bmp280Ok[1] ? "OK" : "FAIL");
#elif I2C_B_TYPE == I2C_BMP180
    bmp180Ok[1] = bmp180[1].begin();
    Serial.printf("[I2C-B] BMP180: %s\n", bmp180Ok[1] ? "OK" : "FAIL");
#endif

    // --- Init probe ---
#if PROBE_TYPE == PROBE_DS18B20
    ds.begin();
    Serial.printf("[PROBE] DS18B20 on GP%d\n", PIN_PROBE);
#elif (PROBE_TYPE == PROBE_DHT11) || (PROBE_TYPE == PROBE_DHT22)
    dht.begin();
    Serial.printf("[PROBE] DHT%d on GP%d\n", PROBE_TYPE == PROBE_DHT11 ? 11 : 22, PIN_PROBE);
#endif

    // Digital inputs — active-low with internal pull-up
    for (int pin : PIN_DIN) pinMode(pin, INPUT_PULLUP);

    // Analog inputs
    for (int pin : PIN_AIN) pinMode(pin, INPUT);

    // Outputs — start all off
    for (int pin : PIN_OUT) { pinMode(pin, OUTPUT); analogWrite(pin, 0); }

    Serial.println("[RepeaterNodeRP2040] ready");
}

// =====================================================================
// LOOP
// =====================================================================
void loop() {
    receiveCommands();

    if (millis() - lastReadMs >= READ_INTERVAL_MS) {
        lastReadMs = millis();
        sendSensorPacket();
    }
}
