#ifndef PID_HPP
#define PID_HPP

#include <Arduino.h>

constexpr float PID_KP_DEFAULT = 1.0F;
constexpr float PID_KI_DEFAULT = 1.0F;
constexpr float PID_KD_DEFAULT = 0.0F;

constexpr float PID_MAX_INTEGRAL = 200.0F;

#endif // PID_HPP