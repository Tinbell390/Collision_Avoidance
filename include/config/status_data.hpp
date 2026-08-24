#ifndef STATUS_DATA_HPP
#define STATUS_DATA_HPP

#include <Arduino.h>

struct StatusData {
    uint32_t timestamp_us;                          // タイムスタンプ（マイクロ秒）
    uint32_t speed_cm_s;                            // 速度（センチメートル毎秒）
    uint32_t target_speed_cm_s;                     // 目標速度（センチメートル毎秒）
    uint32_t line_count;                            // ラインカウント
    uint8_t pwm;                                    // PWM値（0-255）
    uint32_t time_to_enter_intersection_us;         // 交差点に入るまでの時間（マイクロ秒）
    uint32_t time_to_exit_intersection_us;          // 交差点を出るまでの時間（マイクロ秒）
};

#endif // STATUS_DATA_HPP