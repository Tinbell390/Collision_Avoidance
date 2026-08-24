#ifndef CONTROL_HPP
#define CONTROL_HPP

#include <Arduino.h>

// Control
constexpr uint32_t CONTROL_INTERVAL_MS = 10;            // 制御ループの間隔（ミリ秒）
constexpr uint32_t SENSOR_DEBOUNCE_US = 20;            // センサーのデバウンス時間（マイクロ秒）

// Median filter
constexpr uint8_t MEDIAN_FILTER_WINDOW_SIZE = 11;        // メディアンフィルタのウィンドウサイズ（奇数）
constexpr uint8_t SMOOTH_FILTER_WINDOW_SIZE = 10;        // スムージングフィルタのウィンドウサイズ
constexpr bool SMOOTH_FILTER_ENABLED = true;                        // スムージングフィルタの有効化フラグ

// PWM limits
constexpr uint8_t MAX_PWM = 200;                        // 最大PWM値
constexpr uint8_t MIN_PWM = 10;                         // 最小PWM値

#endif // CONTROL_HPP