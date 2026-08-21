#ifndef CONTROL_HPP
#define CONTROL_HPP

#include <Arduino.h>

// Control
constexpr uint32_t CONTROL_INTERVAL_MS = 20;
constexpr uint32_t SENSOR_DEBOUNCE_US = 200;

// Median filter
constexpr uint8_t MEDIAN_FILTER_WINDOW_SIZE = 7;

// PWM limits
constexpr uint8_t MAX_PWM = 200;
constexpr uint8_t MIN_PWM = 10;

#endif // CONTROL_HPP