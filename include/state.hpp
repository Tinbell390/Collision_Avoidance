#ifndef STATE_HPP
#define STATE_HPP

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

extern volatile int32_t sensor_log_index;               // センサーのログインデックス
extern volatile uint32_t line_count;                     // ラインカウント
extern volatile uint8_t current_pwm;                    // 現在のPWM値

extern volatile bool is_started;                        // ビークルがスタートしたかどうかのフラグ
extern volatile bool should_save;                       // ログを保存するかどうかのフラグ

extern volatile uint32_t start_time_us;                 // スタート時間（マイクロ秒）
extern volatile uint32_t last_time_us;                  // センサが最後に更新された時間（マイクロ秒）
extern volatile uint32_t intersection_entry_time_us;          // 交差点に入る時間（マイクロ秒）
extern volatile uint32_t intersection_exit_time_us ;    // 交差点から退出する時間（マイクロ秒）


extern volatile int32_t current_speed_cm_s;             // 現在の速度（センチメートル毎秒）
extern volatile int32_t target_speed_cm_s;              // 目標速度（センチメートル毎秒）
extern volatile int32_t default_target_speed_cm_s;        // 最初の目標速度（センチメートル毎秒）

extern volatile uint32_t self_enter_time_us;                 // 自分の交差点に入る時間（マイクロ秒）
extern volatile uint32_t self_exit_time_us;                  // 自分の交差点を出る時間（マイクロ秒）

extern volatile uint32_t other_enter_time_us;                 // 他の車両の交差点に入る時間（マイクロ秒）
extern volatile uint32_t other_exit_time_us;                  // 他の車両の交差点を出る時間（マイクロ秒）
extern volatile uint32_t other_timestamp_us;                  // 他の車両のタイムスタンプ（マイクロ秒）

extern volatile bool is_running;                        // ビークルが走行中かどうかのフラグ
extern volatile bool is_collision_detected;             // ビークルが意図的衝突モードかどうかのフラグ
extern volatile bool is_lonely;                         // ビークルが独立制御モードかどうかのフラグ
extern volatile bool is_logging;                        // ログを記録中かどうかのフラグ

extern float pid_kp;                                    // PID制御の比例ゲイン
extern float pid_ki;                                    // PID制御の積分ゲイン
extern float pid_kd;                                    // PID制御の微分ゲイン

#endif // STATE_HPP