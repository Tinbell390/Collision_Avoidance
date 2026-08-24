#ifndef SENSOR_H
#define SENSOR_H
#include "config.hpp"
void setup_sensor();
void clear_sensor_interval_buffer();
uint32_t calculate_current_speed_cm_s();

#endif