#ifndef PREDICTION_H
#define PREDICTION_H

#include <Arduino.h>

uint32_t predict_time_to_intersection_us();
uint32_t predict_time_to_exit_intersection_us();

#endif