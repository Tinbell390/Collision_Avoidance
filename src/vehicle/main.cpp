#include <Arduino.h>
#include "config.hpp"
#include "espnow.hpp"
#include "motor.hpp"
#include "sensor.hpp"
#include "controller.hpp"
#include "prediction.hpp"
#include "collision_avoidance.hpp"

// 実験2,実験3で使用する車両用

StatusData current_status;

void setup(){
    Serial.begin(115200);
    setup_esp_now();
    setup_motor();
    setup_sensor();

}

// メインループ
void loop(){
    const uint32_t current_time_us =micros() - start_time_us;

    // 交差点を通過してから一定時間経過したら停止
    if(intersection_entry_time_us != INVALID_TIME_US && current_time_us - intersection_entry_time_us > INTERSECTION_HOLD_TIME_US && is_running){
        brake_motor();
        is_running=false;
    }
    else if(is_running&&current_time_us<RUN_TIME_LIMIT_US){
        //現在の予測到着時間の計算
        self_enter_time_us=predict_time_to_intersection_us();
        self_exit_time_us=predict_time_to_exit_intersection_us();    
        if(other_enter_time_us != INVALID_TIME_US && other_exit_time_us != INVALID_TIME_US&&other_timestamp_us != INVALID_TIME_US){
            target_speed_cm_s = calculate_collision_avoidance_speed_cm_s(current_time_us);
        }

        current_speed_cm_s=calculate_current_speed_cm_s();
        current_pwm = calculate_pid_pwm(current_speed_cm_s, target_speed_cm_s);
        const uint32_t current_interval_us =current_time_us + start_time_us - last_time_us;

        // 交差点を通過したら一定時間走行して停止
        if(line_count>DIST_TO_INTERSECTION_ENTRY_MM/LINE_PITCH_MM){
            intersection_entry_time_us=current_time_us;
        }
        // センサーがタイムアウト時間更新されていなければ停止
        else if(current_interval_us>SENSOR_TIMEOUT_US){
            is_running=false;
            brake_motor();
        }
        else write_motor_pwm(current_pwm);
        // STATUS送信
        current_status.timestamp_us=current_time_us;
        current_status.speed_cm_s = current_speed_cm_s;
        current_status.target_speed_cm_s = target_speed_cm_s;
        current_status.line_count=line_count;
        current_status.pwm=current_pwm; 
        current_status.time_to_enter_intersection_us=self_enter_time_us;
        current_status.time_to_exit_intersection_us=self_exit_time_us;
        send_status(current_status);
    }
    else{
        brake_motor();
    }

    delay(CONTROL_INTERVAL_MS);
}