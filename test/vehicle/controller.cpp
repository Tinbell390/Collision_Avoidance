#include "config.hpp"

float integral=0;
float previousError=0;


//--------------------------------------------------
// コントローラ関数
//--------------------------------------------------
uint8_t PIDcontroller(int currentSpeed, int targetSpeed){
    // currentSpeed[cm/s],targetSpeed[cm/s]であることに注意
    float pwm ;

    // TODO

    return (uint8_t)constrain(pwm,MIN_PWM,MAX_PWM);
}

//--------------------------------------------------
// コントローラ初期化関数
//--------------------------------------------------
void ClearController(){
    integral=0;
    previousError=0;
    targetSpeed = firstspeed;
}