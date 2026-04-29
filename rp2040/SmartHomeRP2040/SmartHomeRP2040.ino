// REQUIRES (install via Arduino IDE Library Manager):
//   DHT sensor library by Adafruit
//   Adafruit Unified Sensor by Adafruit
//
// To use a BME280 instead of DHT22, uncomment USE_BME280 below and also install:
//   Adafruit BME280 Library by Adafruit

// #define USE_BME280

#include "DHT.h"
// #ifdef USE_BME280
// #include <Wire.h>
// #include <Adafruit_BME280.h>
// #endif

// --- UART (inter-chip) ---
const int UART_RX = 0;   // from ESP32 GPIO19 (unused in this sketch)
const int UART_TX = 1;   // to ESP32 GPIO22

// --- Sensor ---
const int DHT_PIN = 22;
#define DHTTYPE DHT22

// --- Timing ---
const unsigned long READ_INTERVAL_MS = 10000UL;

DHT dht(DHT_PIN, DHTTYPE);
// #ifdef USE_BME280
// Adafruit_BME280 bme;
// #endif

unsigned long lastReadMs = 0;

void setup() {
    Serial1.setRX(UART_RX);
    Serial1.setTX(UART_TX);
    Serial1.begin(9600);

    Serial.begin(115200);
    Serial.println("SmartHomeRP2040 ready");

    dht.begin();
    // #ifdef USE_BME280
    // Wire.begin();   // add SDA, SCL pin args if not on default pins — verify from schematic
    // if (!bme.begin(0x76)) {   // try 0x77 if 0x76 fails
    //     Serial.println("[BME280] Init failed — check wiring and I2C address");
    //     while (1);
    // }
    // #endif
}

void readSensor(float &t, float &h) {
#ifndef USE_BME280
    h = dht.readHumidity();
    t = dht.readTemperature();
#else
    // t = bme.readTemperature();
    // h = bme.readHumidity();
    t = NAN;
    h = NAN;
#endif
}

void loop() {
    unsigned long now = millis();
    if (now - lastReadMs < READ_INTERVAL_MS) return;
    lastReadMs = now;

    float t, h;
    readSensor(t, h);

    if (isnan(t) || isnan(h)) {
        Serial.println("[SENSOR] Read failed — skipping");
        return;
    }

    char line[32];
    snprintf(line, sizeof(line), "T:%.2f,H:%.2f\n", t, h);
    Serial1.print(line);
    Serial.print("[UART TX] ");
    Serial.print(line);
}
