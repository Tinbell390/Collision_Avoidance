#include "config.hpp"

float pid_integral=0.0F;
float pid_previous_error=0.0F;

// 課題としてコントローラを設計させる

//--------------------------------------------------
// コントローラ関数
//--------------------------------------------------
uint8_t calculate_pid_pwm(int32_t current_speed_cm_s, int32_t target_speed_cm_s){

    // P制御
    const float error = static_cast<float>(target_speed_cm_s - current_speed_cm_s);
    float P = pid_kp * error;

    // I制御
    pid_integral += error * static_cast<float>(CONTROL_INTERVAL_MS) / 1000 ; 
    pid_integral = constrain(pid_integral,-PID_MAX_INTEGRAL,PID_MAX_INTEGRAL);
    float I = pid_ki * pid_integral;

    // D制御
    float derivative = (error - pid_previous_error) * static_cast<float>(CONTROL_INTERVAL_MS) / 1000 ;
    float D = pid_kd * derivative;
    pid_previous_error = error;

    float pwm = P + I + D;
    pwm = constrain(pwm,MIN_PWM,MAX_PWM);

    return static_cast<uint8_t>(pwm);
}

//--------------------------------------------------
// コントローラ初期化関数
//--------------------------------------------------
void reset_controller(){
    pid_integral=0.0F;
    pid_previous_error=0.0F;
}