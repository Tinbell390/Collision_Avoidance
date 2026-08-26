#ifndef CONFIG_H
#define CONFIG_H
#include <Arduino.h>

#include "hardware.hpp"
#include "state.hpp"

// ============================================================
// 車両パラメータ (Vehicle)
// ============================================================
constexpr int32_t LINE_PITCH_MM                  = 10;    // ラインのピッチ（ミリメートル）
constexpr int32_t VEHICLE_LENGTH_MM              = 150;   // 車両の長さ（ミリメートル）
constexpr uint8_t VEHICLE_COUNT                  = 2;     // ビークルの台数

// 交差点関連の距離
constexpr int32_t DIST_TO_INTERSECTION_ENTRY_MM  = 2500;  // 交差点進入位置までの距離（ミリメートル）
constexpr int32_t DIST_TO_INTERSECTION_EXIT_MM   = 200;   // 交差点退出位置までの距離（ミリメートル）

// ============================================================
// 速度パラメータ (Speed)
// ============================================================
constexpr int32_t DEFAULT_SPEED_CM_S             = 100;   // デフォルト速度（センチメートル/秒）
constexpr int32_t MAX_SPEED_CM_S                 = 200;   // 最大速度（センチメートル/秒）
constexpr int32_t MIN_SPEED_CM_S                 = 20;    // 最小速度（センチメートル/秒）
constexpr uint8_t SPEED_THRESHOLD_CM_S           = 5;     // 到着時間を計算しても良い速度閾値（センチメートル/秒）

// ============================================================
// PID制御 (PID Control)
// ============================================================
constexpr float PID_KP_DEFAULT                   = 0.7F;   // 比例ゲインのデフォルト値
constexpr float PID_KI_DEFAULT                   = 1.1F;   // 積分ゲインのデフォルト値
constexpr float PID_KD_DEFAULT                   = 0.6F;   // 微分ゲインのデフォルト値
constexpr float PID_MAX_INTEGRAL                 = 200.0F; // 積分項の最大値

// ============================================================
// 制御ループ (Control Loop)
// ============================================================
constexpr uint32_t CONTROL_INTERVAL_MS           = 10;     // 制御ループの間隔（ミリ秒）

// ============================================================
// フィルタ (Filters)
// ============================================================
constexpr uint8_t FILTER_WINDOW_SIZE      = 10;     // フィルタのウィンドウサイズ


// ============================================================
// PWM制限 (PWM Limits)
// ============================================================
constexpr uint8_t MAX_PWM                        = 200;    // 最大PWM値
constexpr uint8_t MIN_PWM                        = 10;     // 最小PWM値

// ============================================================
// 時間関連定数 (Timing)
// ============================================================
constexpr uint32_t INVALID_TIME_US               = UINT32_MAX; // 無効な時間を表す定数（マイクロ秒）
constexpr uint32_t TIME_MARGIN_US                = 500000;     // 交差時間のマージン（マイクロ秒）
constexpr uint32_t RUN_TIME_LIMIT_US             = 5000000;    // 走行時間の上限（マイクロ秒）
constexpr uint32_t SENSOR_TIMEOUT_US             = 200000;     // センサーのタイムアウト時間（マイクロ秒）
constexpr uint32_t INTERSECTION_HOLD_TIME_US     = 200000;     // 交差点通過後の保持時間（マイクロ秒）

#endif // CONFIG_H