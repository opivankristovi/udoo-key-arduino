// REQUIRES (install via Arduino IDE Library Manager):
//   PubSubClient by Nick O'Leary
//
// Built-in libraries (no install needed):
//   WiFi.h, I2S.h (ESP32 Arduino core)
//
// Edit config.h with your Wi-Fi credentials and MQTT broker address before flashing.
// The I2S microphone features require the Udoo Key Pro. On a standard Key the I2S
// init will fail gracefully and sensor MQTT will still work normally.

#include "config.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <I2S.h>

// --- UART (inter-chip) ---
const int UART_RX    = 22;   // from RP2040 GPIO1
const int UART_TX    = 19;   // to RP2040 GPIO0 (unused in this sketch)

// --- I2S microphone (Udoo Key Pro only) ---
const int I2S_DATA   = 25;
const int I2S_CLK    = 27;

// --- LEDs ---
const int BLUE_LED   = 32;
const int YELLOW_LED = 33;

// --- Audio analysis ---
const int    SAMPLE_RATE        = 16000;  // Hz
const int    FRAME_SIZE         = 512;    // samples (~32 ms at 16 kHz)
const int    KNOCK_WINDOW_MS    = 1500;   // window to count low-freq impulses
const int    KNOCK_MIN_IMPULSES = 2;      // minimum to classify as knock
const int    KNOCK_MAX_IMPULSES = 5;      // above this → doorbell approximation
// Mean-square energy gate. Quiet room ≈ 1e7–1e8; a knock at 1 m ≈ 5e8–1e10.
// Tune by reading the energy= values in the Serial Monitor during real events.
const double ENERGY_THRESHOLD  = 5e8;
// ZCR bands: < ZCR_KNOCK_MAX → knock/doorbell path; >= ZCR_GLASS_MIN → glass break
const float  ZCR_KNOCK_MAX     = 0.15f;
const float  ZCR_GLASS_MIN     = 0.30f;
const unsigned long DEBOUNCE_MS = 800UL;

// --- Sensor interval ---
const unsigned long SENSOR_TIMEOUT_MS = 60000UL;  // warn if no sensor data for 60 s

// --- Globals ---
WiFiClient   wifiClient;
PubSubClient mqtt(wifiClient);

static int32_t audioFrame[FRAME_SIZE];
static int     frameIdx        = 0;
static int     knockCount      = 0;
static unsigned long knockWindowStart = 0;
static unsigned long lastEventMs      = 0;
static unsigned long lastSensorMs     = 0;

String uartBuffer;
bool   i2sReady = false;

// --- WiFi ---
void connectWiFi() {
    if (WiFi.status() == WL_CONNECTED) return;
    Serial.print("[WiFi] Connecting");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    unsigned long t = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t < 15000UL) {
        delay(500);
        Serial.print('.');
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println(" OK");
    } else {
        Serial.println(" FAILED — will retry");
    }
}

// --- MQTT ---
void connectMQTT() {
    if (mqtt.connected()) return;
    Serial.print("[MQTT] Connecting");
    int attempts = 0;
    while (!mqtt.connected() && attempts < 5) {
        if (mqtt.connect(MQTT_CLIENT)) {
            Serial.println(" OK");
            mqtt.publish(TOPIC_STATUS, "online", /*retain=*/true);
        } else {
            Serial.printf(" failed (rc=%d), retry\n", mqtt.state());
            delay(2000);
        }
        attempts++;
    }
    if (!mqtt.connected()) {
        Serial.println("[MQTT] Could not connect — will retry next loop");
    }
}

// --- UART sensor receive ---
void readUART() {
    while (Serial1.available()) {
        char c = (char)Serial1.read();
        if (c == '\n') {
            if (uartBuffer.length() > 0) {
                parseSensorLine(uartBuffer);
            }
            uartBuffer = "";
        } else {
            uartBuffer += c;
            if (uartBuffer.length() > 64) {
                Serial.println("[UART] Buffer overflow — discarding");
                uartBuffer = "";
            }
        }
    }
}

void parseSensorLine(const String &line) {
    int tIdx = line.indexOf("T:");
    int hIdx = line.indexOf(",H:");
    if (tIdx < 0 || hIdx < 0) {
        Serial.printf("[UART] Unrecognised line: %s\n", line.c_str());
        return;
    }
    float t = line.substring(tIdx + 2, hIdx).toFloat();
    float h = line.substring(hIdx + 3).toFloat();
    if (isnan(t) || isnan(h)) return;

    char buf[12];
    dtostrf(t, 6, 2, buf);
    mqtt.publish(TOPIC_TEMP, buf);
    dtostrf(h, 6, 2, buf);
    mqtt.publish(TOPIC_HUM, buf);

    lastSensorMs = millis();
    digitalWrite(BLUE_LED, !digitalRead(BLUE_LED));  // blink on each sensor publish
    Serial.printf("[SENSOR] T=%.2f H=%.2f published\n", t, h);
}

// --- Audio classification ---
void classifyAudioEvent(double energy, float zcrRate) {
    unsigned long now = millis();
    if (now - lastEventMs < DEBOUNCE_MS) return;

    const char *eventLabel = nullptr;

    if (zcrRate >= ZCR_GLASS_MIN) {
        eventLabel = "glass_break";
        knockCount = 0;
    } else if (zcrRate < ZCR_KNOCK_MAX) {
        // Low-freq impulse — accumulate in knock window
        if (knockCount == 0) {
            knockWindowStart = now;
        }
        knockCount++;

        unsigned long elapsed = now - knockWindowStart;
        if (elapsed >= (unsigned long)KNOCK_WINDOW_MS || knockCount > KNOCK_MAX_IMPULSES) {
            if (knockCount >= KNOCK_MIN_IMPULSES && knockCount <= KNOCK_MAX_IMPULSES) {
                eventLabel = "knock";
            } else if (knockCount > KNOCK_MAX_IMPULSES) {
                eventLabel = "doorbell";
            }
            knockCount = 0;
        }
    } else {
        // Mid-range ZCR
        eventLabel = "loud_event";
        knockCount = 0;
    }

    if (eventLabel != nullptr) {
        Serial.printf("[AUDIO] Event: %s (energy=%.2e ZCR=%.3f)\n",
                      eventLabel, energy, zcrRate);
        if (mqtt.connected()) {
            mqtt.publish(TOPIC_AUDIO, eventLabel);
        }
        digitalWrite(YELLOW_LED, HIGH);
        delay(50);
        digitalWrite(YELLOW_LED, LOW);
        lastEventMs = now;
    }
}

void processAudio() {
    if (!i2sReady) return;

    while (I2S.available() && frameIdx < FRAME_SIZE) {
        audioFrame[frameIdx++] = I2S.read();
    }
    if (frameIdx < FRAME_SIZE) return;
    frameIdx = 0;

    double energy = 0.0;
    int    zcr    = 0;
    int32_t prev  = audioFrame[0];

    for (int i = 1; i < FRAME_SIZE; i++) {
        int32_t s = audioFrame[i];
        energy += (double)s * s;
        if ((prev > 0 && s <= 0) || (prev < 0 && s >= 0)) zcr++;
        prev = s;
    }
    energy /= FRAME_SIZE;
    float zcrRate = (float)zcr / FRAME_SIZE;

    if (energy >= ENERGY_THRESHOLD) {
        classifyAudioEvent(energy, zcrRate);
    }
}

// --- Setup & loop ---
void setup() {
    pinMode(BLUE_LED, OUTPUT);
    pinMode(YELLOW_LED, OUTPUT);
    digitalWrite(BLUE_LED, LOW);
    digitalWrite(YELLOW_LED, LOW);

    Serial.begin(115200);
    Serial1.begin(9600, SERIAL_8N1, UART_RX, UART_TX);

    connectWiFi();
    mqtt.setServer(MQTT_HOST, MQTT_PORT);
    mqtt.setKeepAlive(60);
    connectMQTT();

    I2S.setDataPin(I2S_DATA);
    I2S.setSckPin(I2S_CLK);
    if (I2S.begin(esp_i2s::I2S_MODE_MASTER, SAMPLE_RATE, 32)) {
        i2sReady = true;
        Serial.println("[AUDIO] I2S microphone ready");
    } else {
        Serial.println("[AUDIO] I2S init failed — audio detection disabled (Udoo Key Pro required)");
    }

    Serial.println("SmartHomeESP32 ready");
}

void loop() {
    if (WiFi.status() != WL_CONNECTED) connectWiFi();
    if (!mqtt.connected()) connectMQTT();
    mqtt.loop();

    readUART();
    processAudio();
}
