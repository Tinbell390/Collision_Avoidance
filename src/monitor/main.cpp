#include <Arduino.h>
#include "config.hpp"
#include "espnow.hpp"
#include "logger.hpp"

// 実験2,実験3で使用するモニタ用
// 

void setup(){
    Serial.begin(115200);
    setup_ESPNOW();
    set_Monitor();
    setup_logger();
}

// メインループ
void loop(){
    if (!Serial.available()) return;

    // コマンド入力
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd == "start"){
        SendSTART(false);
        OpenFile();
        Serial.println("Send START (CollisionFlag = false)");
    }
    else if (cmd == "start collision"){
        SendSTART(true);
        Serial.println("Send START (CollisionFlag = true)");
    }
    else if (cmd.startsWith("speed ")){
        // "speed "以降を整数に変換
        int speed = cmd.substring(6).toInt();
        SendSPEED(speed);
        Serial.printf("Send SET_SPEED (%d)\n", speed);
    }
    else if (cmd.startsWith("gain ")){
        float kp, ki, kd;

        // "gain KP KI KD" を解析
        int ret = sscanf(cmd.c_str(), "gain %f %f %f", &kp, &ki, &kd);

        if (ret == 3){
            SendGAIN(kp, ki, kd);
            Serial.printf("Send GAIN (KP=%.3f, KI=%.3f, KD=%.3f)\n",kp, ki, kd);
        }
        else{
            Serial.println("Usage: gain <KP> <KI> <KD>");
        }
    }
    else if (cmd == "finish"){
        SendFINISH();
        CloseFile();
        sendLog();
        Serial.println("Send FINISH");
    }
    else{
        Serial.println("Unknown command");
    }
}