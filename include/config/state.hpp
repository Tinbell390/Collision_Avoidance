#ifndef STATE_HPP
#define STATE_HPP

#include <Arduino.h>

extern volatile int32_t sensor_log_index;
extern volatile int32_t line_count;
extern volatile uint8_t current_pwm;

extern volatile bool is_started;
extern volatile bool should_save;

extern volatile uint32_t start_time_us;
extern volatile uint32_t last_time_us;

extern volatile int32_t current_speed_cm_s;
extern volatile int32_t target_speed_cm_s;
extern volatile int32_t first_target_speed_cm_s;

extern volatile uint32_t enter_time_us;
extern volatile uint32_t exit_time_us;

extern volatile bool is_running;
extern volatile bool is_collision_detected;
extern volatile bool is_lonely;
extern volatile bool is_logging;

extern float pid_kp;
extern float pid_ki;
extern float pid_kd;

#endif // STATE_HPP