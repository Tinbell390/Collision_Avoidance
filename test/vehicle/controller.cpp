#include "config.hpp"

float pid_integral=0.0F;            //累積偏差
float pid_previous_error=0.0F;      //前回の偏差

// 課題としてコントローラを設計させる

//--------------------------------------------------
// コントローラ関数
//--------------------------------------------------
uint8_t calculate_pid_pwm(int32_t current_speed_cm_s, int32_t target_speed_cm_s){

    // P制御


    // I制御


    // D制御


    // PWM出力（MIN_PWM～MAX_PWMの範囲に制限）


}

//--------------------------------------------------
// コントローラ初期化関数
//--------------------------------------------------
void reset_controller(){
    pid_integral=0.0F;
    pid_previous_error=0.0F;
}

//--------------------------------------------------
// PIDcontrollerを設計せよ
//
// 各ゲインはKP，KI，KDで定義される。
// 1フレームの時間はCONTROL_INTERVAL_MS[ms]で定義されている。
// 
// current_speed_cm_s, target_speed_cm_sは[cm/s]であることに注意すること。
//
// MIN_PWM～MAX_PWMの範囲に制限して出力すること。
//
// 累積偏差（積分値）が±MAX_INTEGRALを超えないよう制限すること。
//
// 比例項・積分項・微分項を用いてPID制御を実装せよ。
//--------------------------------------------------