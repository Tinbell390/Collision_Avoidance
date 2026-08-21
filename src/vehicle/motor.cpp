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
    ledcSetup(PWM_CHANNEL, PWM_FREQUENCY_HZ, PWM_RESOLUTION_BITS);
    ledcAttachPin(ENABLE_PIN, PWM_CHANNEL);

    brake_motor();
}

//--------------------------------------------------
// モータ速度関数
//--------------------------------------------------
void write_motor_pwm(uint8_t pwm){
    ledcWrite(PWM_CHANNEL, pwm);
}

//--------------------------------------------------
// モータブレーキ関数
//--------------------------------------------------
void brake_motor(){
    // モータドライバにショートブレーキを指示する
    ledcWrite(PWM_CHANNEL,0);
    current_pwm=0;
}

