#ifndef TIME_HPP
#define TIME_HPP

#include <Arduino.h>

constexpr uint32_t INVALID_TIME_US = UINT32_MAX;            // 無効な時間を表す定数（マイクロ秒）

constexpr uint32_t TIME_MARGIN_US = 50000;                  // 交差時間のマージン（マイクロ秒）
constexpr uint32_t RUN_TIME_LIMIT_US = 300000000;           // 走行時間の上限（マイクロ秒）
constexpr uint32_t SENSOR_TIMEOUT_MS = 500000;          // センサーのタイムアウト時間（マイクロ秒）      

#endif // TIME_HPP