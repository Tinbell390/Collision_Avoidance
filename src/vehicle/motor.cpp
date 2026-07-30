#include <Arduino.h>
#include "config.hpp"
#include "motor.hpp"

//--------------------------------------------------
// モータセットアップ関数
//--------------------------------------------------
void setup_motor(){
    //ピンをセット
    pinMode(PHASE_PIN, OUTPUT);
    pinMode(SENSOR_PIN, INPUT);
    if(RUN_SWITCH_PIN) pinMode(RUN_SWITCH_PIN,OUTPUT);

    if(RUN_SWITCH_PIN)digitalWrite(RUN_SWITCH_PIN,HIGH);

    // 回転方向
    digitalWrite(PHASE_PIN, LOW);

    // PWMをセット
    ledcSetup(ledcChannel, freq, resolution);
    ledcAttachPin(ENABLE_PIN, ledcChannel);

    motorBrake();
}

//--------------------------------------------------
// モータ速度関数
//--------------------------------------------------
void motorWrite(int duty){
    ledcWrite(ledcChannel, duty);
}

//--------------------------------------------------
// モータブレーキ関数
//--------------------------------------------------
void motorBrake(){
    // モータドライバにショートブレーキを指示する
    ledcWrite(ledcChannel,0);
}

