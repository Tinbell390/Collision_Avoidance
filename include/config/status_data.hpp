#ifndef STATUS_DATA_HPP
#define STATUS_DATA_HPP

#include <Arduino.h>

struct StatusData {
    uint32_t timestamp_us;
    uint32_t speed_cm_s;
    uint8_t pwm;
    uint32_t time_to_enter_intersection_us;
    uint32_t time_to_exit_intersection_us;
};

#endif // STATUS_DATA_HPP