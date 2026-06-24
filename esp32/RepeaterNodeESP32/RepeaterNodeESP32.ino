/*
 * RepeaterNodeESP32 — WiFi NAT repeater + MQTT hub, Udoo Key ESP32 side.
 *
 * - Bridges an upstream Wi-Fi network (STA) to a local access point using
 *   NAT (true repeater — AP clients can reach the internet)
 * - Receives JSON sensor packets from the RP2040 over the on-board UART bridge
 * - Publishes each sensor value to MQTT under <TOPIC_BASE>/sensor/...
 * - Subscribes to <TOPIC_BASE>/output/<N>/set and forwards output commands
 *   to the RP2040 as C:{json}\n
 *
 * Counterpart sketch: rp2040/RepeaterNodeRP2040
 *
 * Board:     ESP32 Dev Module
 * Monitor:   115200 baud (USB Serial)
 * Bridge:    9600 baud (Serial1, GPIO22/19 → RP2040 GP1/GP0)
 * Libraries: PubSubClient, ArduinoJson v7
 *
 * Fill in config.h with your credentials before flashing.
 *
 * UART protocol (newline-terminated lines):
 *   RP2040 → ESP32:  S:{json}\n   — sensor packet
 *   ESP32  → RP2040: C:{json}\n   — output command: {"o":<0-3>,"v":<0-255>}
 *
 * Published MQTT topics (under TOPIC_BASE):
 *   status                      — online / offline (retained LWT)
 *   sensor/i2c_a/{temperature,humidity,pressure}
 *   sensor/i2c_b/{temperature,humidity,pressure}
 *   sensor/probe/{temperature,humidity}
 *   sensor/analog/{1,2}         — raw ADC value 0-4095
 *   input/{1,2}/state           — ON / OFF (retained)
 *   output/{1-4}/state          — ON / OFF (retained, updated on command)
 *
 * Subscribed topics:
 *   output/{1-4}/set            — ON | OFF | 0-255
 */

#include "config.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// NAPT include path differs between Arduino-ESP32 core 2 and 3
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  #include "lwip/lwip_napt.h"
#else
  #include "lwip/napt.h"
  #include "lwip/dns.h"
#endif

// =====================================================================
// HARDWARE
// =====================================================================
static const int UART_RX_PIN = 22;   // from RP2040 GP1 (TX)
static const int UART_TX_PIN = 19;   // to   RP2040 GP0 (RX)
static const int UART_BAUD   = 9600;
static const int LED_BLUE    = 32;
static const int LED_YELLOW  = 33;

// =====================================================================
// MQTT STATE
// =====================================================================
static WiFiClient   wifiClient;
static PubSubClient mqtt(wifiClient);

// =====================================================================
// UART RECEIVE BUFFER
// =====================================================================
static String rxBuf;
static bool   rxOverflow = false;

// =====================================================================
// HELPERS
// =====================================================================

static void pub(const char* subtopic, const char* val, bool retain = false) {
    char t[72];
    snprintf(t, sizeof(t), TOPIC_BASE "/%s", subtopic);
    mqtt.publish(t, val, retain);
}

static void pubF(const char* subtopic, float v) {
    char buf[16];
    dtostrf(v, 0, 2, buf);
    pub(subtopic, buf);
}

// =====================================================================
// WI-FI + NAPT
// =====================================================================

static void enableNapt() {
    ip_napt_enable(WiFi.softAPIP().addr, 1);
    Serial.println("[NAPT] enabled");
}

static void connectSta() {
    WiFi.begin(WIFI_STA_SSID, WIFI_STA_PASS);
    Serial.print("[WiFi] connecting to " WIFI_STA_SSID);
    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000UL) {
        delay(400);
        Serial.print('.');
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf(" OK  IP=%s\n", WiFi.localIP().toString().c_str());
        enableNapt();
    } else {
        Serial.println(" TIMEOUT — will retry in loop");
    }
}

static void wifiSetup() {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS);
    Serial.printf("[WiFi] AP up: %s  %s\n", WIFI_AP_SSID,
                  WiFi.softAPIP().toString().c_str());
    connectSta();
}

static void wifiMaintain() {
    if (WiFi.status() == WL_CONNECTED) return;
    Serial.println("[WiFi] STA lost — reconnecting");
    WiFi.disconnect();
    connectSta();
}

// =====================================================================
// MQTT
// =====================================================================

static void handleOutputSet(const char* rawTopic, const String& payload) {
    // Topic format:  udookey/output/<N>/set  (N is 1-based)
    // Find the output number between the last two slashes before "/set"
    String t(rawTopic);
    int lastSlash = t.lastIndexOf('/');
    int prevSlash = t.lastIndexOf('/', lastSlash - 1);
    if (lastSlash < 0 || prevSlash < 0) return;
    int n = t.substring(prevSlash + 1, lastSlash).toInt();
    if (n < 1 || n > 4) return;

    // Parse value: ON → 255, OFF → 0, numeric string → 0-255
    int val;
    String p = payload;
    p.trim();
    if (p.equalsIgnoreCase("ON"))       val = 255;
    else if (p.equalsIgnoreCase("OFF")) val = 0;
    else                                val = constrain(p.toInt(), 0, 255);

    // Publish retained state
    char stateSub[24];
    snprintf(stateSub, sizeof(stateSub), "output/%d/state", n);
    pub(stateSub, val > 0 ? "ON" : "OFF", true);

    // Forward to RP2040
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "C:{\"o\":%d,\"v\":%d}\n", n - 1, val);
    Serial1.print(cmd);
    Serial.printf("[CMD] output%d = %d\n", n, val);
}

static void onMqttMessage(char* rawTopic, byte* payload, unsigned int len) {
    String p;
    for (unsigned int i = 0; i < len; i++) p += (char)payload[i];

    // Only handle output/N/set topics
    String t(rawTopic);
    String pfx = String(TOPIC_BASE "/output/");
    if (t.startsWith(pfx) && t.endsWith("/set"))
        handleOutputSet(rawTopic, p);
}

static void mqttConnect() {
    if (mqtt.connected() || WiFi.status() != WL_CONNECTED) return;
    Serial.print("[MQTT] connecting...");
    const char* u  = strlen(MQTT_USER) ? MQTT_USER : nullptr;
    const char* pw = strlen(MQTT_PASS) ? MQTT_PASS : nullptr;
    if (mqtt.connect(MQTT_CLIENT, u, pw,
                     TOPIC_BASE "/status", 0, true, "offline")) {
        Serial.println(" OK");
        mqtt.publish(TOPIC_BASE "/status", "online", true);
        for (int i = 1; i <= 4; i++) {
            char sub[32];
            snprintf(sub, sizeof(sub), TOPIC_BASE "/output/%d/set", i);
            mqtt.subscribe(sub);
        }
    } else {
        Serial.printf(" FAIL rc=%d\n", mqtt.state());
    }
}

// =====================================================================
// SENSOR PACKET PARSING
// =====================================================================

static void parseSensorPacket(const String& json) {
    JsonDocument doc;
    if (deserializeJson(doc, json)) {
        Serial.printf("[UART] JSON error in: %s\n", json.c_str());
        return;
    }

    // I2C device A — publish only fields that exist in the packet
    float iaT = doc["ia"]["t"] | NAN, iaH = doc["ia"]["h"] | NAN,
          iaP = doc["ia"]["p"] | NAN;
    if (!isnan(iaT)) pubF("sensor/i2c_a/temperature", iaT);
    if (!isnan(iaH)) pubF("sensor/i2c_a/humidity",    iaH);
    if (!isnan(iaP)) pubF("sensor/i2c_a/pressure",    iaP);

    // I2C device B
    float ibT = doc["ib"]["t"] | NAN, ibH = doc["ib"]["h"] | NAN,
          ibP = doc["ib"]["p"] | NAN;
    if (!isnan(ibT)) pubF("sensor/i2c_b/temperature", ibT);
    if (!isnan(ibH)) pubF("sensor/i2c_b/humidity",    ibH);
    if (!isnan(ibP)) pubF("sensor/i2c_b/pressure",    ibP);

    // Probe
    float prT = doc["pr"]["t"] | NAN, prH = doc["pr"]["h"] | NAN;
    if (!isnan(prT)) pubF("sensor/probe/temperature", prT);
    if (!isnan(prH)) pubF("sensor/probe/humidity",    prH);

    // Analog inputs — raw ADC value (0-4095)
    int a1 = doc["an"][0] | -1, a2 = doc["an"][1] | -1;
    if (a1 >= 0) pub("sensor/analog/1", String(a1).c_str());
    if (a2 >= 0) pub("sensor/analog/2", String(a2).c_str());

    // Digital inputs — retained (represent current state)
    for (int i = 0; i < 2; i++) {
        int v = doc["di"][i] | -1;
        if (v < 0) continue;
        char sub[24];
        snprintf(sub, sizeof(sub), "input/%d/state", i + 1);
        pub(sub, v ? "ON" : "OFF", true);
    }

    // Visual feedback
    digitalWrite(LED_BLUE, !digitalRead(LED_BLUE));
    Serial.println("[SENSOR] published");
}

// =====================================================================
// UART READ
// =====================================================================
static void readUART() {
    while (Serial1.available()) {
        char c = (char)Serial1.read();
        if (c == '\n' || c == '\r') {
            if (c == '\n' && !rxOverflow && rxBuf.length() > 2) {
                if (rxBuf.startsWith("S:"))
                    parseSensorPacket(rxBuf.substring(2));
                else
                    Serial.printf("[UART] unknown: %s\n", rxBuf.c_str());
            }
            rxBuf      = "";
            rxOverflow = false;
        } else {
            rxBuf += c;
            if (rxBuf.length() > 512) {
                rxBuf      = "";
                rxOverflow = true;
                Serial.println("[UART] overflow — discarding");
            }
        }
    }
}

// =====================================================================
// SETUP & LOOP
// =====================================================================
void setup() {
    pinMode(LED_BLUE,   OUTPUT); digitalWrite(LED_BLUE,   LOW);
    pinMode(LED_YELLOW, OUTPUT); digitalWrite(LED_YELLOW, LOW);

    Serial.begin(115200);
    delay(200);
    Serial.println("\n[RepeaterNodeESP32] starting");

    Serial1.begin(UART_BAUD, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);

    wifiSetup();

    mqtt.setServer(MQTT_HOST, MQTT_PORT);
    mqtt.setCallback(onMqttMessage);
    mqtt.setBufferSize(512);

    mqttConnect();

    Serial.printf("[RepeaterNodeESP32] ready — AP: %s  %s\n",
                  WIFI_AP_SSID, WiFi.softAPIP().toString().c_str());
}

void loop() {
    wifiMaintain();
    if (!mqtt.connected()) mqttConnect();
    mqtt.loop();
    readUART();
}
