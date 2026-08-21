#ifndef HARDWARE_HPP
#define HARDWARE_HPP

#include <Arduino.h>

// Hardware
constexpr int PHASE_PIN = D5;
constexpr int ENABLE_PIN = D3;
constexpr int SENSOR_PIN = D6;
constexpr int RUN_SWITCH_PIN = D2;

// PWM
constexpr int PWM_CHANNEL = 0;
constexpr int PWM_FREQUENCY_HZ = 20000;
constexpr int PWM_RESOLUTION_BITS = 8;

#endif // HARDWARE_HPP