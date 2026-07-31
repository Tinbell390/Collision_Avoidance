#include "config.hpp"

float integral=0;
float previousError=0;


//--------------------------------------------------
// コントローラ関数
//--------------------------------------------------
uint8_t PIDcontroller(int currentSpeed, int targetSpeed){
    // currentSpeed[cm/s],targetSpeed[cm/s],積分値は±MAX_INTEGRALの範囲を超えてはならない
    float pwm ;
    float dt = INTERVAL_MS/1000;

    // TODO

    return (uint8_t)constrain(pwm,MIN_PWM,MAX_PWM);
}

//--------------------------------------------------
// コントローラ初期化関数
//--------------------------------------------------
void ClearController(){
    integral=0;
    previousError=0;
    targetSpeed = FirstTargetSpeed;
}