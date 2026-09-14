#include "config.hpp"

float pid_integral=0.0F;            //累積偏差
float pid_previous_error=0.0F;      //前回の偏差

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
// calculate_pid_pwm関数を設計・実装せよ
//
// ・本課題では、1フレーム単位の離散時間PID制御を用いる。
//
// ・PIDゲインには pid_kp，pid_ki，pid_kd を使用する。
//
// ・current_speed_cm_s と target_speed_cm_s は [cm/s] の値である。
//
// ・比例項，積分項，微分項を用いてPWM出力を計算する。
//
// ・積分値は pid_integral に累積し、-PID_MAX_INTEGRAL ～ PID_MAX_INTEGRAL の範囲に制限する。
//
// ・微分項の計算には、前フレームの偏差を保持するpid_previous_error を使用する。
//
// ・計算したPWM出力は、MIN_PWM ～ MAX_PWM の範囲に制限する。
//
// ・最終的なPWM値を uint8_t で返す。
// 
//--------------------------------------------------
