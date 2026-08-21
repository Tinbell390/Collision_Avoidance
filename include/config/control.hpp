#ifndef CONTROL_HPP
#define CONTROL_HPP

#include <Arduino.h>

// Control
constexpr uint32_t CONTROL_INTERVAL_MS = 20;            // 制御ループの間隔（ミリ秒）
constexpr uint32_t SENSOR_DEBOUNCE_US = 200;            // センサーのデバウンス時間（マイクロ秒）

// Median filter
constexpr uint8_t MEDIAN_FILTER_WINDOW_SIZE = 7;        // メディアンフィルタのウィンドウサイズ（奇数）

// PWM limits
constexpr uint8_t MAX_PWM = 200;                        // 最大PWM値
constexpr uint8_t MIN_PWM = 10;                         // 最小PWM値

#endif // CONTROL_HPP