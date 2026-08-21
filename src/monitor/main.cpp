#include <Arduino.h>
#include "config.hpp"
#include "espnow.hpp"
#include "logger.hpp"

// 実験2,実験3で使用するモニタ用
// 

void setup(){
    Serial.begin(115200);
    setup_esp_now();
    setup_logger();
}

// メインループ
void loop(){
    if (!Serial.available()) return;

    // コマンド入力
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd == "start"){
        send_start(false,false);
        open_log_file();
        Serial.println("Send START (CollisionFlag = false)");
    }
    else if (cmd == "start collision"){
        send_start(true,false);
        open_log_file();
        Serial.println("Send START (CollisionFlag = true)");
    }
    else if (cmd == "start lonery"){
        send_start(false,true);
        open_log_file();
        Serial.println("Send START");
    }    
    else if (cmd.startsWith("speed ")){
        // "speed "以降を整数に変換
        int speed = cmd.substring(6).toInt();
        send_speed(speed);
        Serial.printf("Send SET_SPEED (%d)\n", speed);
    }
    else if (cmd.startsWith("gain ")){
        float kp, ki, kd;

        // "gain KP KI KD" を解析
        int ret = sscanf(cmd.c_str(), "gain %f %f %f", &kp, &ki, &kd);

        if (ret == 3){
            send_gain(kp, ki, kd);
            Serial.printf("Send GAIN (KP=%.3f, KI=%.3f, KD=%.3f)\n",kp, ki, kd);
        }
        else{
            Serial.println("Usage: gain <KP> <KI> <KD>");
        }
    }
    else if (cmd == "finish"){
        send_finish();
        close_log_file();
        send_log();
        Serial.println("Send FINISH");
    }
    else{
        Serial.println("Unknown command");
    }
}