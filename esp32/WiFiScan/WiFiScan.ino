#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  delay(500);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  Serial.println("WiFi scanner ready.");
}

void loop() {
  Serial.println("Scanning...");

  int n = WiFi.scanNetworks();

  if (n == 0) {
    Serial.println("No networks found.");
  } else {
    Serial.print(n);
    Serial.println(" network(s) found:");
    for (int i = 0; i < n; i++) {
      Serial.printf("  %2d. %-32s  RSSI: %4d dBm  %s\n",
        i + 1,
        WiFi.SSID(i).c_str(),
        WiFi.RSSI(i),
        (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "open" : "secured");
    }
  }

  WiFi.scanDelete();
  Serial.println();
  delay(5000);
}
