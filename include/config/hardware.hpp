#ifndef HARDWARE_HPP
#define HARDWARE_HPP

#include <Arduino.h>

// Hardware
constexpr int PHASE_PIN = D5;                   // モータの回転方向を制御するピン
constexpr int ENABLE_PIN = D3;                  // モータの有効化を制御するピン
constexpr int SENSOR_PIN = D6;                  // センサー接続ピン
constexpr int RUN_SWITCH_PIN = D2;              // ランスイッチ接続ピン

// PWM
constexpr int PWM_CHANNEL = 0;                  // PWMチャンネル番号
constexpr int PWM_FREQUENCY_HZ = 20000;         // PWM周波数（Hz）
constexpr int PWM_RESOLUTION_BITS = 8;          // PWM分解能（ビット数）

#endif // HARDWARE_HPP