/*
 * RepeaterNodeESP32 v2 — WiFi NAT repeater + MQTT hub + web portal, Udoo Key ESP32.
 *
 * New in v2:
 *   - Captive-portal web page: configure Wi-Fi, MQTT, NTP and clock schedules
 *     at runtime — no re-flashing required. Join the AP, browse 192.168.4.1.
 *   - Home Assistant auto-discovery (retained MQTT config topics).
 *   - NTP time sync + per-output clock-schedule control.
 *   - Settings persisted in NVS (Preferences); config.h provides first-boot defaults.
 *
 * Board:     ESP32 Dev Module
 * Monitor:   115200 baud (USB Serial)
 * Bridge:    9600 baud (Serial1, GPIO22/19 → RP2040 GP1/GP0)
 * Libraries: PubSubClient, ArduinoJson v7
 *            (WebServer, DNSServer, Preferences ship with the ESP32 core)
 *
 * First-run: join "UdooKey-AP" / "repeater1", browse to 192.168.4.1.
 *
 * UART protocol (newline-terminated, 9600 baud):
 *   RP2040 → ESP32:  S:{json}\n   — sensor packet
 *   ESP32  → RP2040: C:{"o":<0-3>,"v":<0-255>}\n — output command
 */

#include "config.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <time.h>

#if ESP_ARDUINO_VERSION_MAJOR >= 3
  #include "lwip/lwip_napt.h"
#else
  #include "lwip/napt.h"
  #include "lwip/dns.h"
#endif

// ─── Hardware ────────────────────────────────────────────────────────────────
static const int UART_RX_PIN = 22;
static const int UART_TX_PIN = 19;
static const int UART_BAUD   = 9600;
static const int LED_BLUE    = 32;
static const int LED_YELLOW  = 33;

// ─── Settings struct ─────────────────────────────────────────────────────────
struct Schedule {
    bool    enabled      = false;
    char    onTime[6]    = "07:00";  // "HH:MM"
    char    offTime[6]   = "22:00";
    uint8_t val          = 255;
};

struct Settings {
    char     mqttHost[64]   = MQTT_HOST;
    uint16_t mqttPort       = MQTT_PORT;
    char     mqttUser[32]   = MQTT_USER;
    char     mqttPass[32]   = MQTT_PASS;
    char     mqttClient[24] = MQTT_CLIENT;
    char     topicBase[32]  = TOPIC_BASE;
    bool     haEnabled      = true;
    char     haPrefix[32]   = "homeassistant";
    char     ntpServer[64]  = "pool.ntp.org";
    char     timezone[64]   = "UTC0";
    Schedule schedule[4];
};

// ─── Global state ────────────────────────────────────────────────────────────
static Settings cfg;
static char     staSsid[33], staPass[65], apSsid[33], apPass[65];
static bool     ntpSynced    = false;
static int      sentOnMin[4]  = {-1, -1, -1, -1};
static int      sentOffMin[4] = {-1, -1, -1, -1};

static WiFiClient   wifiClient;
static PubSubClient mqtt(wifiClient);
static WebServer    server(80);
static DNSServer    dns;

static String rxBuf;
static bool   rxOverflow = false;

// ─── NVS load / save ─────────────────────────────────────────────────────────
static void loadWifiCreds() {
    Preferences p; p.begin("repeater", true);
    strlcpy(staSsid, p.getString("sta_ssid", WIFI_STA_SSID).c_str(), sizeof(staSsid));
    strlcpy(staPass, p.getString("sta_pass",  WIFI_STA_PASS).c_str(), sizeof(staPass));
    strlcpy(apSsid,  p.getString("ap_ssid",  WIFI_AP_SSID).c_str(),  sizeof(apSsid));
    strlcpy(apPass,  p.getString("ap_pass",  WIFI_AP_PASS).c_str(),  sizeof(apPass));
    p.end();
}

static void saveWifiCreds() {
    Preferences p; p.begin("repeater", false);
    p.putString("sta_ssid", staSsid);
    p.putString("sta_pass",  staPass);
    p.putString("ap_ssid",  apSsid);
    p.putString("ap_pass",  apPass);
    p.end();
}

static void loadSettings() {
    Preferences p; p.begin("settings", true);
    String json = p.getString("json", "");
    p.end();
    if (!json.length()) return;

    JsonDocument doc;
    if (deserializeJson(doc, json)) return;

    strlcpy(cfg.mqttHost,   doc["mh"]  | MQTT_HOST,       sizeof(cfg.mqttHost));
    cfg.mqttPort =           doc["mp"]  | (int)MQTT_PORT;
    strlcpy(cfg.mqttUser,   doc["mu"]  | MQTT_USER,       sizeof(cfg.mqttUser));
    strlcpy(cfg.mqttPass,   doc["mpw"] | MQTT_PASS,       sizeof(cfg.mqttPass));
    strlcpy(cfg.mqttClient, doc["mc"]  | MQTT_CLIENT,     sizeof(cfg.mqttClient));
    strlcpy(cfg.topicBase,  doc["tb"]  | TOPIC_BASE,      sizeof(cfg.topicBase));
    cfg.haEnabled =          doc["ha"]  | true;
    strlcpy(cfg.haPrefix,   doc["hap"] | "homeassistant", sizeof(cfg.haPrefix));
    strlcpy(cfg.ntpServer,  doc["ntp"] | "pool.ntp.org",  sizeof(cfg.ntpServer));
    strlcpy(cfg.timezone,   doc["tz"]  | "UTC0",          sizeof(cfg.timezone));

    JsonArray sa = doc["sc"].as<JsonArray>();
    for (int i = 0; i < 4 && i < (int)sa.size(); i++) {
        cfg.schedule[i].enabled = sa[i]["en"] | false;
        strlcpy(cfg.schedule[i].onTime,  sa[i]["on"]  | "07:00", 6);
        strlcpy(cfg.schedule[i].offTime, sa[i]["off"] | "22:00", 6);
        cfg.schedule[i].val = sa[i]["v"] | 255;
    }
}

static void saveSettings() {
    JsonDocument doc;
    doc["mh"]  = cfg.mqttHost;
    doc["mp"]  = cfg.mqttPort;
    doc["mu"]  = cfg.mqttUser;
    doc["mpw"] = cfg.mqttPass;
    doc["mc"]  = cfg.mqttClient;
    doc["tb"]  = cfg.topicBase;
    doc["ha"]  = cfg.haEnabled;
    doc["hap"] = cfg.haPrefix;
    doc["ntp"] = cfg.ntpServer;
    doc["tz"]  = cfg.timezone;
    JsonArray sa = doc["sc"].to<JsonArray>();
    for (int i = 0; i < 4; i++) {
        JsonObject o = sa.add<JsonObject>();
        o["en"]  = cfg.schedule[i].enabled;
        o["on"]  = cfg.schedule[i].onTime;
        o["off"] = cfg.schedule[i].offTime;
        o["v"]   = cfg.schedule[i].val;
    }
    char buf[768]; serializeJson(doc, buf, sizeof(buf));
    Preferences p; p.begin("settings", false);
    p.putString("json", buf);
    p.end();
}

// ─── Web portal ──────────────────────────────────────────────────────────────

static const char PAGE_HEAD[] PROGMEM =
    "<!DOCTYPE html><html><head><meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>Udoo Key Repeater</title>"
    "<style>"
    "body{font-family:sans-serif;margin:0;background:#1a1a2e;color:#eee}"
    "h1{background:#16213e;padding:12px 16px;margin:0;font-size:1.1em}"
    "nav{background:#0f3460;padding:6px 16px;display:flex;gap:8px;flex-wrap:wrap}"
    "nav a{color:#e94560;text-decoration:none;padding:4px 8px;border-radius:4px}"
    "nav a:hover{background:#e94560;color:#fff}"
    ".c{padding:16px;max-width:620px}"
    "label{display:block;margin-top:10px;font-size:.9em;color:#aaa}"
    "input[type=text],input[type=password],input[type=number],input[type=time],select"
    "{width:100%;padding:6px;background:#16213e;border:1px solid #444;color:#eee;"
    "border-radius:4px;box-sizing:border-box;margin-top:4px}"
    "input[type=checkbox]{width:auto;margin-right:6px}"
    ".cb{display:flex;align-items:center;margin-top:10px;font-size:.9em;color:#aaa}"
    ".btn{margin-top:14px;padding:8px 18px;background:#e94560;color:#fff;"
    "border:none;border-radius:4px;cursor:pointer}"
    ".btn:hover{background:#c73652}"
    ".btn-g{background:#444}.btn-g:hover{background:#666}"
    ".row{display:flex;gap:12px}.row>*{flex:1}"
    "h3{color:#e94560;margin-top:0}"
    "h4{color:#aaa;margin:16px 0 4px}"
    ".ok{color:#4caf50}.warn{color:#ff9800}.err{color:#f44336}"
    "table{width:100%;border-collapse:collapse;font-size:.9em}"
    "td,th{padding:6px 8px;border-bottom:1px solid #333;text-align:left}"
    "th{color:#aaa}"
    "small{color:#666}"
    "</style></head><body>"
    "<h1>Udoo Key Repeater Node</h1>"
    "<nav>"
    "<a href='/'>Status</a>"
    "<a href='/network'>Network</a>"
    "<a href='/mqtt'>MQTT</a>"
    "<a href='/system'>System</a>"
    "<a href='/schedules'>Schedules</a>"
    "</nav><div class='c'>";

static const char PAGE_FOOT[] PROGMEM = "</div></body></html>";

static String pageHead() { return FPSTR(PAGE_HEAD); }
static String pageFoot() { return FPSTR(PAGE_FOOT); }

static String fld(const char* id, const char* label, const char* val,
                   const char* type = "text") {
    String s = "<label>"; s += label;
    s += "<input type='"; s += type;
    s += "' name='"; s += id;
    s += "' value='"; s += val;
    s += "'></label>";
    return s;
}

static String fldN(const char* id, const char* label, int val) {
    char v[12]; snprintf(v, sizeof(v), "%d", val);
    return fld(id, label, v, "number");
}

static String cb(const char* id, const char* label, bool checked) {
    String s = "<label class='cb'><input type='checkbox' name='"; s += id;
    s += "' value='1'";
    if (checked) s += " checked";
    s += "> "; s += label; s += "</label>";
    return s;
}

static void rebootPage(const char* msg) {
    String h = pageHead();
    h += "<h3>Saving\xe2\x80\xa6</h3><p>"; h += msg;
    h += "</p><p>Rebooting \xe2\x80\x94 <a href='/'>refresh</a> in ~5 s.</p>";
    h += pageFoot();
    server.send(200, "text/html", h);
    delay(800);
    ESP.restart();
}

static void handleRoot() {
    String h = pageHead();
    h += "<h3>Status</h3><table>";
    h += "<tr><th>AP</th><td>"; h += apSsid;
    h += " &nbsp; "; h += WiFi.softAPIP().toString(); h += "</td></tr>";
    h += "<tr><th>STA</th><td>";
    if (WiFi.status() == WL_CONNECTED) {
        h += "<span class='ok'>Connected \xe2\x80\x94 ";
        h += WiFi.localIP().toString();
        h += " &nbsp; RSSI "; h += WiFi.RSSI(); h += " dBm</span>";
    } else {
        h += "<span class='err'>Disconnected</span>";
    }
    h += "</td></tr>";
    h += "<tr><th>MQTT</th><td>";
    h += mqtt.connected() ? "<span class='ok'>Connected</span>"
                          : "<span class='err'>Disconnected</span>";
    h += "</td></tr>";
    h += "<tr><th>NTP</th><td>";
    if (ntpSynced) {
        struct tm t; getLocalTime(&t, 0);
        char ts[32]; strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &t);
        h += "<span class='ok'>"; h += ts; h += "</span>";
    } else {
        h += "<span class='warn'>Not synced</span>";
    }
    h += "</td></tr><tr><th>Uptime</th><td>";
    h += (millis() / 1000); h += " s</td></tr></table>";
    h += "<form method='post' action='/reset' style='margin-top:20px'>"
         "<button class='btn btn-g' "
         "onclick=\"return confirm('Clear all settings and reboot?')\">"
         "Factory Reset</button></form>";
    h += pageFoot();
    server.send(200, "text/html", h);
}

static void handleReset() {
    server.send(200, "text/html",
                pageHead() + "<h3>Resetting\xe2\x80\xa6</h3>"
                "<p>All settings cleared. Rebooting.</p>" + pageFoot());
    delay(400);
    Preferences p;
    p.begin("repeater", false); p.clear(); p.end();
    p.begin("settings", false); p.clear(); p.end();
    delay(400); ESP.restart();
}

static void handleNetwork() {
    String h = pageHead();
    h += "<h3>Network</h3><form method='post' action='/save/network'>";
    h += "<h4>Upstream Wi-Fi (STA)</h4>";
    h += fld("ss", "SSID", staSsid);
    h += fld("sp", "Password", staPass, "password");
    h += "<h4>Repeater Access Point</h4>";
    h += fld("as", "AP SSID", apSsid);
    h += fld("ap", "AP Password (min 8 chars)", apPass, "password");
    h += "<br><button class='btn'>Save &amp; Reboot</button></form>";
    h += pageFoot(); server.send(200, "text/html", h);
}

static void handleSaveNetwork() {
    String ss = server.arg("ss"), sp = server.arg("sp");
    String as_ = server.arg("as"), ap_ = server.arg("ap");
    if (!ss.length() || !as_.length() || ap_.length() < 8) {
        server.send(400, "text/html",
            pageHead() + "<p class='err'>Invalid input. AP password must be &ge;8 chars.</p>"
            "<a href='/network'>Back</a>" + pageFoot());
        return;
    }
    strlcpy(staSsid, ss.c_str(),  sizeof(staSsid));
    strlcpy(staPass, sp.c_str(),  sizeof(staPass));
    strlcpy(apSsid,  as_.c_str(), sizeof(apSsid));
    strlcpy(apPass,  ap_.c_str(), sizeof(apPass));
    saveWifiCreds();
    rebootPage("Network settings saved.");
}

static void handleMqtt() {
    String h = pageHead();
    h += "<h3>MQTT</h3><form method='post' action='/save/mqtt'>";
    h += "<div class='row'>";
    h += fld("mh", "Broker host", cfg.mqttHost);
    h += fldN("mp", "Port", cfg.mqttPort);
    h += "</div>";
    h += fld("mu",  "Username (optional)", cfg.mqttUser);
    h += fld("mpw", "Password (optional)", cfg.mqttPass, "password");
    h += fld("mc",  "Client ID", cfg.mqttClient);
    h += fld("tb",  "Topic base", cfg.topicBase);
    h += "<h4>Home Assistant</h4>";
    h += cb("ha", "Enable HA auto-discovery", cfg.haEnabled);
    h += fld("hap", "Discovery prefix", cfg.haPrefix);
    h += "<br><button class='btn'>Save &amp; Reboot</button></form>";
    h += pageFoot(); server.send(200, "text/html", h);
}

static void handleSaveMqtt() {
    strlcpy(cfg.mqttHost,   server.arg("mh").c_str(),  sizeof(cfg.mqttHost));
    cfg.mqttPort =           server.arg("mp").toInt();
    strlcpy(cfg.mqttUser,   server.arg("mu").c_str(),  sizeof(cfg.mqttUser));
    strlcpy(cfg.mqttPass,   server.arg("mpw").c_str(), sizeof(cfg.mqttPass));
    strlcpy(cfg.mqttClient, server.arg("mc").c_str(),  sizeof(cfg.mqttClient));
    strlcpy(cfg.topicBase,  server.arg("tb").c_str(),  sizeof(cfg.topicBase));
    cfg.haEnabled =          server.hasArg("ha");
    strlcpy(cfg.haPrefix,   server.arg("hap").c_str(), sizeof(cfg.haPrefix));
    if (!cfg.mqttPort) cfg.mqttPort = 1883;
    saveSettings();
    rebootPage("MQTT settings saved.");
}

static void handleSystem() {
    String h = pageHead();
    h += "<h3>System / NTP</h3><form method='post' action='/save/system'>";
    h += fld("ntp", "NTP server", cfg.ntpServer);
    h += "<label>Timezone (POSIX string)"
         "<br><small>Examples: UTC0 &nbsp; "
         "CET-1CEST,M3.5.0,M10.5.0/3 &nbsp; "
         "EST5EDT,M3.2.0,M11.1.0 &nbsp; "
         "AEST-10AEDT,M10.1.0,M4.1.0/3</small>"
         "<input type='text' name='tz' value='";
    h += cfg.timezone; h += "'></label>";
    h += "<br><button class='btn'>Save &amp; Reboot</button></form>";
    h += pageFoot(); server.send(200, "text/html", h);
}

static void handleSaveSystem() {
    strlcpy(cfg.ntpServer, server.arg("ntp").c_str(), sizeof(cfg.ntpServer));
    strlcpy(cfg.timezone,  server.arg("tz").c_str(),  sizeof(cfg.timezone));
    saveSettings();
    rebootPage("System settings saved.");
}

static void handleSchedules() {
    String h = pageHead();
    h += "<h3>Clock Schedules</h3>"
         "<p><small>NTP must be synced. Each output turns ON at the on-time "
         "and OFF at the off-time, every day.</small></p>"
         "<form method='post' action='/save/schedules'>";
    for (int i = 0; i < 4; i++) {
        h += "<h4>Output "; h += (i + 1); h += "</h4>";
        char id[8];
        snprintf(id, sizeof(id), "en%d", i);
        h += cb(id, "Enable schedule", cfg.schedule[i].enabled);
        h += "<div class='row'>";
        snprintf(id, sizeof(id), "on%d", i);
        h += fld(id, "Turn ON at", cfg.schedule[i].onTime, "time");
        snprintf(id, sizeof(id), "of%d", i);
        h += fld(id, "Turn OFF at", cfg.schedule[i].offTime, "time");
        snprintf(id, sizeof(id), "vl%d", i);
        h += fldN(id, "ON level (0-255)", cfg.schedule[i].val);
        h += "</div>";
    }
    h += "<br><button class='btn'>Save &amp; Reboot</button></form>";
    h += pageFoot(); server.send(200, "text/html", h);
}

static void handleSaveSchedules() {
    for (int i = 0; i < 4; i++) {
        char id[8];
        snprintf(id, sizeof(id), "en%d", i); cfg.schedule[i].enabled = server.hasArg(id);
        snprintf(id, sizeof(id), "on%d", i); strlcpy(cfg.schedule[i].onTime,  server.arg(id).c_str(), 6);
        snprintf(id, sizeof(id), "of%d", i); strlcpy(cfg.schedule[i].offTime, server.arg(id).c_str(), 6);
        snprintf(id, sizeof(id), "vl%d", i); cfg.schedule[i].val = constrain(server.arg(id).toInt(), 0, 255);
    }
    saveSettings();
    rebootPage("Schedule settings saved.");
}

static void handleNotFound() {
    server.sendHeader("Location", "http://192.168.4.1/");
    server.send(302, "text/plain", "");
}

static void portalSetup() {
    dns.start(53, "*", WiFi.softAPIP());
    server.on("/",               HTTP_GET,  handleRoot);
    server.on("/network",        HTTP_GET,  handleNetwork);
    server.on("/save/network",   HTTP_POST, handleSaveNetwork);
    server.on("/mqtt",           HTTP_GET,  handleMqtt);
    server.on("/save/mqtt",      HTTP_POST, handleSaveMqtt);
    server.on("/system",         HTTP_GET,  handleSystem);
    server.on("/save/system",    HTTP_POST, handleSaveSystem);
    server.on("/schedules",      HTTP_GET,  handleSchedules);
    server.on("/save/schedules", HTTP_POST, handleSaveSchedules);
    server.on("/reset",          HTTP_POST, handleReset);
    // Captive-portal OS detection endpoints
    server.on("/generate_204",       []{ server.send(204, "text/plain", ""); });
    server.on("/hotspot-detect.html", handleRoot);
    server.on("/ncsi.txt",           []{ server.send(200, "text/plain", "Microsoft NCSI"); });
    server.onNotFound(handleNotFound);
    server.begin();
}

// ─── Wi-Fi + NAPT ────────────────────────────────────────────────────────────
static void startNtp();   // forward declaration

static void enableNapt() {
    ip_napt_enable((uint32_t)WiFi.softAPIP(), 1);
    Serial.println("[NAPT] enabled");
}

static void connectSta() {
    WiFi.begin(staSsid, staPass);
    Serial.printf("[WiFi] connecting to %s", staSsid);
    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000UL) {
        delay(400); Serial.print('.');
        server.handleClient(); dns.processNextRequest();
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf(" OK  IP=%s\n", WiFi.localIP().toString().c_str());
        enableNapt();
        startNtp();
    } else {
        Serial.println(" TIMEOUT — will retry in loop");
    }
}

static void wifiSetup() {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(apSsid, apPass);
    Serial.printf("[WiFi] AP up: %s  %s\n", apSsid,
                  WiFi.softAPIP().toString().c_str());
    connectSta();
}

static void wifiMaintain() {
    static bool wasConn = false;
    bool now = (WiFi.status() == WL_CONNECTED);
    if (!now && wasConn) {
        Serial.println("[WiFi] STA lost — reconnecting");
        WiFi.disconnect();
        ntpSynced = false;
        connectSta();
    }
    wasConn = now;
}

// ─── NTP ─────────────────────────────────────────────────────────────────────
static void startNtp() {
    configTzTime(cfg.timezone, cfg.ntpServer);
    ntpSynced = false;
    Serial.printf("[NTP] configured: %s  tz=%s\n", cfg.ntpServer, cfg.timezone);
}

static void checkNtp() {
    if (ntpSynced || WiFi.status() != WL_CONNECTED) return;
    struct tm t;
    if (!getLocalTime(&t, 0)) return;
    ntpSynced = true;
    char ts[32]; strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &t);
    Serial.printf("[NTP] synced: %s\n", ts);
}

// ─── MQTT helpers ────────────────────────────────────────────────────────────
static void pub(const char* sub, const char* val, bool retain = false) {
    char t[80]; snprintf(t, sizeof(t), "%s/%s", cfg.topicBase, sub);
    mqtt.publish(t, val, retain);
}

static void pubF(const char* sub, float v) {
    char buf[16]; dtostrf(v, 0, 2, buf); pub(sub, buf);
}

static void sendOutput(int idx, int val) {
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "C:{\"o\":%d,\"v\":%d}\n", idx, val);
    Serial1.print(cmd);
    char sub[24];
    snprintf(sub, sizeof(sub), "output/%d/state", idx + 1);
    pub(sub, val > 0 ? "ON" : "OFF", true);
    Serial.printf("[CMD] output%d = %d\n", idx + 1, val);
}

// ─── HA discovery ────────────────────────────────────────────────────────────
static void publishHaEntity(const char* domain, const char* key,
                              const char* name,  const char* device_class,
                              const char* unit,  const char* state_topic,
                              const char* cmd_topic = nullptr,
                              const char* payload_on  = nullptr,
                              const char* payload_off = nullptr) {
    char cfgTopic[128], uid[56];
    snprintf(uid,      sizeof(uid),      "%s_%s",             cfg.mqttClient, key);
    snprintf(cfgTopic, sizeof(cfgTopic), "%s/%s/%s/config",   cfg.haPrefix, domain, uid);

    if (!cfg.haEnabled) {
        mqtt.publish(cfgTopic, "", true);
        return;
    }

    JsonDocument doc;
    doc["name"]                  = name;
    doc["unique_id"]             = uid;
    doc["state_topic"]           = state_topic;
    doc["availability_topic"]    = String(cfg.topicBase) + "/status";
    doc["payload_available"]     = "online";
    doc["payload_not_available"] = "offline";
    if (device_class && strlen(device_class)) doc["device_class"]         = device_class;
    if (unit         && strlen(unit))          doc["unit_of_measurement"]  = unit;
    if (cmd_topic)                             doc["command_topic"]        = cmd_topic;
    if (payload_on)                            doc["payload_on"]           = payload_on;
    if (payload_off)                           doc["payload_off"]          = payload_off;
    doc["device"]["name"]           = "Udoo Key Repeater";
    doc["device"]["identifiers"][0] = cfg.mqttClient;
    doc["device"]["model"]          = "Udoo Key";
    doc["device"]["manufacturer"]   = "UDOO";

    char buf[640]; serializeJson(doc, buf, sizeof(buf));
    mqtt.publish(cfgTopic, buf, true);
}

static void publishHaDiscovery() {
    const String b = cfg.topicBase;

    // I2C sensor A
    publishHaEntity("sensor", "i2c_a_temp", "I2C-A Temperature",
                    "temperature", "\xc2\xb0\x43", (b+"/sensor/i2c_a/temperature").c_str());
    publishHaEntity("sensor", "i2c_a_hum",  "I2C-A Humidity",
                    "humidity",    "%",  (b+"/sensor/i2c_a/humidity").c_str());
    publishHaEntity("sensor", "i2c_a_pres", "I2C-A Pressure",
                    "pressure",    "hPa",(b+"/sensor/i2c_a/pressure").c_str());

    // I2C sensor B
    publishHaEntity("sensor", "i2c_b_temp", "I2C-B Temperature",
                    "temperature", "\xc2\xb0\x43", (b+"/sensor/i2c_b/temperature").c_str());
    publishHaEntity("sensor", "i2c_b_hum",  "I2C-B Humidity",
                    "humidity",    "%",  (b+"/sensor/i2c_b/humidity").c_str());
    publishHaEntity("sensor", "i2c_b_pres", "I2C-B Pressure",
                    "pressure",    "hPa",(b+"/sensor/i2c_b/pressure").c_str());

    // Probe
    publishHaEntity("sensor", "probe_temp", "Probe Temperature",
                    "temperature", "\xc2\xb0\x43", (b+"/sensor/probe/temperature").c_str());
    publishHaEntity("sensor", "probe_hum",  "Probe Humidity",
                    "humidity",    "%",  (b+"/sensor/probe/humidity").c_str());

    // Analog inputs
    publishHaEntity("sensor", "analog_1", "Analog Input 1", "", "", (b+"/sensor/analog/1").c_str());
    publishHaEntity("sensor", "analog_2", "Analog Input 2", "", "", (b+"/sensor/analog/2").c_str());

    // Digital inputs (binary_sensor)
    for (int i = 1; i <= 2; i++) {
        char key[16], name[24], st[72];
        snprintf(key,  sizeof(key),  "input_%d",          i);
        snprintf(name, sizeof(name), "Digital Input %d",  i);
        snprintf(st,   sizeof(st),   "%s/input/%d/state", cfg.topicBase, i);
        publishHaEntity("binary_sensor", key, name, "", "", st, nullptr, "ON", "OFF");
    }

    // Outputs (switch)
    for (int i = 1; i <= 4; i++) {
        char key[16], name[24], st[72], cmd[72];
        snprintf(key,  sizeof(key),  "output_%d",              i);
        snprintf(name, sizeof(name), "Output %d",              i);
        snprintf(st,   sizeof(st),   "%s/output/%d/state",     cfg.topicBase, i);
        snprintf(cmd,  sizeof(cmd),  "%s/output/%d/set",       cfg.topicBase, i);
        publishHaEntity("switch", key, name, "", "", st, cmd, "ON", "OFF");
    }
    Serial.println("[HA] discovery published (14 entities)");
}

// ─── MQTT connect / callback ──────────────────────────────────────────────────
static void handleOutputSet(const char* rawTopic, const String& payload) {
    String t(rawTopic);
    int ls = t.lastIndexOf('/'), ps = t.lastIndexOf('/', ls - 1);
    if (ls < 0 || ps < 0) return;
    int n = t.substring(ps + 1, ls).toInt();
    if (n < 1 || n > 4) return;

    String p = payload; p.trim();
    int val;
    if (p.equalsIgnoreCase("ON"))       val = 255;
    else if (p.equalsIgnoreCase("OFF")) val = 0;
    else                                val = constrain(p.toInt(), 0, 255);

    sendOutput(n - 1, val);
}

static void onMqttMessage(char* rawTopic, byte* payload, unsigned int len) {
    String p; for (unsigned int i = 0; i < len; i++) p += (char)payload[i];
    String t(rawTopic), pfx = String(cfg.topicBase) + "/output/";
    if (t.startsWith(pfx) && t.endsWith("/set"))
        handleOutputSet(rawTopic, p);
}

static void mqttConnect() {
    if (mqtt.connected() || WiFi.status() != WL_CONNECTED) return;
    Serial.print("[MQTT] connecting...");
    char lwt[80]; snprintf(lwt, sizeof(lwt), "%s/status", cfg.topicBase);
    const char* u  = strlen(cfg.mqttUser) ? cfg.mqttUser : nullptr;
    const char* pw = strlen(cfg.mqttPass) ? cfg.mqttPass : nullptr;
    if (mqtt.connect(cfg.mqttClient, u, pw, lwt, 0, true, "offline")) {
        Serial.println(" OK");
        mqtt.publish(lwt, "online", true);
        for (int i = 1; i <= 4; i++) {
            char sub[72];
            snprintf(sub, sizeof(sub), "%s/output/%d/set", cfg.topicBase, i);
            mqtt.subscribe(sub);
        }
        publishHaDiscovery();
    } else {
        Serial.printf(" FAIL rc=%d\n", mqtt.state());
    }
}

// ─── Clock schedules ──────────────────────────────────────────────────────────
static int parseTime(const char* hhmm) {
    int h = 0, m = 0;
    if (sscanf(hhmm, "%d:%d", &h, &m) == 2 && h >= 0 && h < 24 && m >= 0 && m < 60)
        return h * 60 + m;
    return -1;
}

static void checkSchedules() {
    if (!ntpSynced) return;
    struct tm t;
    if (!getLocalTime(&t, 0)) return;
    int now = t.tm_hour * 60 + t.tm_min;
    for (int i = 0; i < 4; i++) {
        if (!cfg.schedule[i].enabled) continue;
        int onM  = parseTime(cfg.schedule[i].onTime);
        int offM = parseTime(cfg.schedule[i].offTime);
        if (onM  >= 0 && now == onM  && sentOnMin[i]  != onM) {
            sentOnMin[i] = onM;
            sendOutput(i, cfg.schedule[i].val);
            Serial.printf("[SCHED] output%d ON at %s\n", i+1, cfg.schedule[i].onTime);
        }
        if (offM >= 0 && now == offM && sentOffMin[i] != offM) {
            sentOffMin[i] = offM;
            sendOutput(i, 0);
            Serial.printf("[SCHED] output%d OFF at %s\n", i+1, cfg.schedule[i].offTime);
        }
    }
}

// ─── Sensor packet parser ─────────────────────────────────────────────────────
static void parseSensorPacket(const String& json) {
    JsonDocument doc;
    if (deserializeJson(doc, json)) {
        Serial.printf("[UART] JSON err: %s\n", json.c_str()); return;
    }

    float iaT = doc["ia"]["t"] | NAN, iaH = doc["ia"]["h"] | NAN, iaP = doc["ia"]["p"] | NAN;
    if (!isnan(iaT)) pubF("sensor/i2c_a/temperature", iaT);
    if (!isnan(iaH)) pubF("sensor/i2c_a/humidity",    iaH);
    if (!isnan(iaP)) pubF("sensor/i2c_a/pressure",    iaP);

    float ibT = doc["ib"]["t"] | NAN, ibH = doc["ib"]["h"] | NAN, ibP = doc["ib"]["p"] | NAN;
    if (!isnan(ibT)) pubF("sensor/i2c_b/temperature", ibT);
    if (!isnan(ibH)) pubF("sensor/i2c_b/humidity",    ibH);
    if (!isnan(ibP)) pubF("sensor/i2c_b/pressure",    ibP);

    float prT = doc["pr"]["t"] | NAN, prH = doc["pr"]["h"] | NAN;
    if (!isnan(prT)) pubF("sensor/probe/temperature", prT);
    if (!isnan(prH)) pubF("sensor/probe/humidity",    prH);

    int a1 = doc["an"][0] | -1, a2 = doc["an"][1] | -1;
    if (a1 >= 0) pub("sensor/analog/1", String(a1).c_str());
    if (a2 >= 0) pub("sensor/analog/2", String(a2).c_str());

    for (int i = 0; i < 2; i++) {
        int v = doc["di"][i] | -1; if (v < 0) continue;
        char sub[24]; snprintf(sub, sizeof(sub), "input/%d/state", i + 1);
        pub(sub, v ? "ON" : "OFF", true);
    }

    digitalWrite(LED_BLUE, !digitalRead(LED_BLUE));
    Serial.println("[SENSOR] published");
}

// ─── UART reader ──────────────────────────────────────────────────────────────
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
            rxBuf = ""; rxOverflow = false;
        } else {
            rxBuf += c;
            if (rxBuf.length() > 512) {
                rxBuf = ""; rxOverflow = true;
                Serial.println("[UART] overflow");
            }
        }
    }
}

// ─── Setup & loop ─────────────────────────────────────────────────────────────
void setup() {
    pinMode(LED_BLUE,   OUTPUT); digitalWrite(LED_BLUE,   LOW);
    pinMode(LED_YELLOW, OUTPUT); digitalWrite(LED_YELLOW, LOW);

    Serial.begin(115200);
    delay(200);
    Serial.println("\n[RepeaterNodeESP32] starting v2");

    loadSettings();
    loadWifiCreds();

    Serial1.begin(UART_BAUD, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);

    wifiSetup();
    portalSetup();

    mqtt.setServer(cfg.mqttHost, cfg.mqttPort);
    mqtt.setCallback(onMqttMessage);
    mqtt.setBufferSize(1024);

    mqttConnect();

    Serial.printf("[RepeaterNodeESP32] ready — AP: %s  %s\n",
                  apSsid, WiFi.softAPIP().toString().c_str());
    Serial.printf("[Portal] http://%s/\n", WiFi.softAPIP().toString().c_str());
}

void loop() {
    dns.processNextRequest();
    server.handleClient();
    wifiMaintain();
    if (!mqtt.connected()) mqttConnect();
    mqtt.loop();
    readUART();
    checkNtp();
    checkSchedules();
}
