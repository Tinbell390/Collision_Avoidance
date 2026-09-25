#include <Arduino.h>
#include "esp_system.h"

void setup() {
  Serial.begin(115200);
  delay(1000);
}

void loop() {
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);

  Serial.printf(
    "MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
    mac[0], mac[1], mac[2],
    mac[3], mac[4], mac[5]
  );

  delay(1000);
}