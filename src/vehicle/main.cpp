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

// 内部条件による停止判定。
// is_running は開始指令/強制停止指令によって外部(espnowコールバック等)からも
// 書き換えられるため、ここでは「false にする」ことだけ行い、
// モーター停止処理自体は loop() 側で一括して行う。
void check_internal_stop_conditions(uint32_t current_time_us){
    // センサーが一定時間更新されていなければ停止(交差点通過後の待機中も対象)
    const uint32_t current_interval_us = micros() - last_time_us;
    if(current_interval_us > SENSOR_TIMEOUT_US){
        is_running = false;
        return;
    }

    // 交差点を通過してから保持時間が経過したら停止
    if(intersection_entry_time_us != INVALID_TIME_US &&
       current_time_us - intersection_entry_time_us > INTERSECTION_HOLD_TIME_US){
        is_running = false;
        return;
    }

    // 走行時間の上限に達したら停止
    if(current_time_us >= RUN_TIME_LIMIT_US){
        is_running = false;
        return;
    }
}

// 走行中の速度・PWM計算、交差点進入検知、モーター制御、ステータス送信
void run_control(uint32_t current_time_us){
    // 交差点への進入検知(一度検知したら上書きしない)
    if(intersection_entry_time_us == INVALID_TIME_US &&
       line_count > DIST_TO_INTERSECTION_ENTRY_MM / LINE_PITCH_MM){
        intersection_entry_time_us = current_time_us;
    }

    // 現在の予測到着時間の計算
    self_enter_time_us = predict_time_to_intersection_us();
    self_exit_time_us  = predict_time_to_exit_intersection_us();

    if(other_enter_time_us != INVALID_TIME_US &&
       other_exit_time_us  != INVALID_TIME_US &&
       other_timestamp_us  != INVALID_TIME_US){
        target_speed_cm_s = calculate_collision_avoidance_speed_cm_s(current_time_us);
    }

    current_speed_cm_s = calculate_current_speed_cm_s();
    current_pwm = calculate_pid_pwm(current_speed_cm_s, target_speed_cm_s);
    write_motor_pwm(current_pwm);

    // STATUS送信
    current_status.timestamp_us = current_time_us;
    current_status.speed_cm_s = current_speed_cm_s;
    current_status.target_speed_cm_s = target_speed_cm_s;
    current_status.line_count = line_count;
    current_status.pwm = current_pwm;
    current_status.time_to_enter_intersection_us = self_enter_time_us;
    current_status.time_to_exit_intersection_us = self_exit_time_us;
    send_status(current_status);
}

// メインループ
void loop(){
    const uint32_t current_time_us = micros() - start_time_us;

    // 開始/強制停止は外部(通信)からも is_running を直接書き換えるので、
    // ここでは走行中の場合にのみ内部条件による停止判定を追加で行う
    if(is_running){
        check_internal_stop_conditions(current_time_us);
    }

    if(is_running){
        run_control(current_time_us);
    }
    else{
        brake_motor();
    }

    delay(CONTROL_INTERVAL_MS);
}