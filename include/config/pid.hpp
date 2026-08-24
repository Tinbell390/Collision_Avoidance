#ifndef PID_HPP
#define PID_HPP

#include <Arduino.h>

constexpr float PID_KP_DEFAULT = 0.5F;                    // PID制御の比例ゲインのデフォルト値
constexpr float PID_KI_DEFAULT = 5.0F;                    // PID制御の積分ゲインのデフォルト値
constexpr float PID_KD_DEFAULT = 0.0F;                    // PID制御の微分ゲインのデフォルト値

constexpr float PID_MAX_INTEGRAL = 200.0F;                // PID制御の積分項の最大値

#endif // PID_HPP