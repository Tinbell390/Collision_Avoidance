#ifndef CONTROLLER_H
#define CONTROLLER_H

uint8_t calculate_pid_pwm(int32_t current_speed_cm_s, int32_t target_speed_cm_s);
void reset_controller();

#endif