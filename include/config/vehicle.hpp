#ifndef VEHICLE_HPP
#define VEHICLE_HPP

#include <Arduino.h>

// Vehicle parameters
constexpr int32_t LINE_PITCH_MM = 10;                       // ラインのピッチ（ミリメートル）
constexpr int32_t DIST_TO_INTERSECTION_ENTRY_MM = 2000;     // 交差点進入位置までの距離（ミリメートル）
constexpr int32_t DIST_TO_INTERSECTION_EXIT_MM = 200;       // 交差点退出位置までの距離（ミリメートル）
constexpr int32_t VEHICLE_LENGTH_MM = 150;                  // 車両の長さ（ミリメートル）

constexpr int32_t DEFAULT_SPEED_CM_S = 100;                 // デフォルト速度（センチメートル/秒）
constexpr int32_t MAX_SPEED_CM_S = 400;                     // 最大速度（センチメートル/秒）
constexpr int32_t MIN_SPEED_CM_S = 20;                      // 最小速度（センチメートル/秒）

// Vehicle
constexpr uint8_t VEHICLE_COUNT = 2;                        // ビークルの台数
constexpr uint8_t SPEED_THRESHOLD_CM_S = 20;                // 到着時間を計算しても良い速度閾値（センチメートル/秒）

#endif // VEHICLE_HPP