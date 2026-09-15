#include <Arduino.h>

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
            Serial.println("HELLO FROM ESP32-C3");
        }
    }
}