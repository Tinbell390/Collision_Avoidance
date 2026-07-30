#include "config.hpp"

float integral=0;
float previousError=0;

// 課題としてコントローラを設計させる

//--------------------------------------------------
// コントローラ関数
//--------------------------------------------------
uint8_t PIDcontroller(int currentSpeed, int targetSpeed){

    // P制御
    float error = targetSpeed - currentSpeed;
    float P = KP * error;

    // I制御
    integral += error * INTERVAL_MS / 1000 ; 
    integral = constrain(integral,-MAX_INTEGRAL,MAX_INTEGRAL);
    float I = KI * integral;

    // D制御
    float derivative = (error - previousError) *1000/ INTERVAL_MS ;
    float D = KD * derivative;
    previousError = error;

    float pwm = P + I + D;
    pwm = constrain(pwm,MIN_PWM,MAX_PWM);

    return (uint8_t)pwm;
}

//--------------------------------------------------
// コントローラ初期化関数
//--------------------------------------------------
void ClearController(){
    integral=0;
    previousError=0;
    targetSpeed = firstspeed;
}