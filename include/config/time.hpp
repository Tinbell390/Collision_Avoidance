#ifndef TIME_HPP
#define TIME_HPP

#include <Arduino.h>

constexpr uint32_t INVALID_TIME_US = UINT32_MAX;

constexpr uint32_t TIME_MARGIN_US = 50000;
constexpr uint32_t RUN_TIME_LIMIT_US = 300000000;

#endif // TIME_HPP