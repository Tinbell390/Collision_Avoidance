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

//--------------------------------------------------
// PIDcontrollerを設計せよ
//
// currentSpeed, targetSpeedは[cm/s]であることに注意すること。
// 速度偏差からPID制御によるPWM値を計算し、
// MIN_PWM～MAX_PWMの範囲に制限して出力すること。
//
// 積分項はdtを用いて更新し、
// 積分値が±MAX_INTEGRALを超えないよう制限すること。
//
// 各ゲインはKP，KI，KDで定義される。
// 比例項・積分項・微分項を用いてPID制御を実装せよ。
//--------------------------------------------------