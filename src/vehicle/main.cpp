#include <Arduino.h>
#include "config.hpp"
#include "espnow.hpp"
#include "motor.hpp"
#include "sensor.hpp"
#include "controller.hpp"
#include "prediction.hpp"

// 実験2,実験3で使用する車両用

StatusData current_status;

void setup(){
    Serial.begin(115200);
    setup_esp_now();
    setup_motor();
    setup_sensor();

}

// メインループ
void loop()
{
    const uint32_t current_time_us =micros() - start_time_us;
    if(is_running&&current_time_us<RUN_TIME_LIMIT_US){
        current_speed_cm_s=calculate_current_speed_cm_s();
        current_pwm = calculate_pid_pwm(current_speed_cm_s, target_speed_cm_s);
        // 交差点を通過したら停止
        if(line_count>DIST_TO_INTERSECTION_ENTRY_MM/LINE_PITCH_MM){
            brake_motor();
        }
        else write_motor_pwm(current_pwm);
        // STATUS送信
        current_status.timestamp_us=current_time_us;
        current_status.speed_cm_s = current_speed_cm_s;
        current_status.pwm=current_pwm; 
        enter_time_us=predict_time_to_intersection_us();
        exit_time_us=predict_time_to_exit_intersection_us();     
        current_status.time_to_enter_intersection_us=enter_time_us;
        current_status.time_to_exit_intersection_us=exit_time_us;
        send_status(current_status);
    }
    else{
        brake_motor();
    }

    delay(CONTROL_INTERVAL_MS);
}