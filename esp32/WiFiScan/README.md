# WiFiScan

Scans for nearby Wi-Fi networks every 5 seconds and prints each network's SSID, signal strength (RSSI), and whether it is open or secured to the Serial Monitor.

No credentials or network connection are required — useful for verifying that the ESP32 Wi-Fi radio is working and for exploring the RF environment.

## Board selection

In Arduino IDE select **ESP32 Dev Module** as the target board.

## Libraries

Uses the built-in `WiFi.h` library included with the ESP32 Arduino core — no additional installation needed.

## Serial output

Open the Serial Monitor at **115200 baud**.

Example output:
```
Scanning...
4 network(s) found:
   1. MyHomeNetwork                   RSSI:  -45 dBm  secured
   2. NeighbourWifi                   RSSI:  -72 dBm  secured
   3. CoffeeShop_Guest                RSSI:  -80 dBm  open
   4. AndroidAP                       RSSI:  -85 dBm  secured
```
