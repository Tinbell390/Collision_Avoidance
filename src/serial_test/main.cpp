#include <Arduino.h>
#include "esp_system.h"

void setup()
{
    Serial.begin(115200);
}

void loop()
{
    // シリアルデータを受信したか確認
    if (Serial.available() > 0)
    {
        // 改行まで読み込む
        String message = Serial.readStringUntil('\n');

        // 前後の空白・改行を削除
        message.trim();

        // HELLOを受信したら応答
        if (message == "HELLO")
        {
            // MACアドレスを取得
            uint8_t mac[6];
            esp_read_mac(mac, ESP_MAC_WIFI_STA);

            // 応答
            Serial.println("HELLO FROM ESP32-C3");

            Serial.printf(
                "MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
                mac[0],
                mac[1],
                mac[2],
                mac[3],
                mac[4],
                mac[5]
            );
        }
    }
}