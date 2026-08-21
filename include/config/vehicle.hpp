#ifndef VEHICLE_HPP
#define VEHICLE_HPP

#include <Arduino.h>

// Vehicle parameters
constexpr int32_t LINE_PITCH_MM = 10;
constexpr int32_t DIST_TO_INTERSECTION_ENTRY_MM = 2000;
constexpr int32_t DIST_TO_INTERSECTION_EXIT_MM = 200;
constexpr int32_t VEHICLE_LENGTH_MM = 150;

constexpr int32_t DEFAULT_SPEED_CM_S = 100;
constexpr int32_t MAX_SPEED_CM_S = 400;
constexpr int32_t MIN_SPEED_CM_S = 20;

// Vehicle
constexpr uint8_t VEHICLE_COUNT = 2;
constexpr uint8_t SPEED_THRESHOLD_CM_S = 20;

#endif // VEHICLE_HPP