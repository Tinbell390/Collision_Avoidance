#include "config.hpp"

volatile int32_t sensor_log_index = 0;
volatile uint32_t line_count = 0;

volatile uint8_t current_pwm = 0;

volatile bool is_started = false;
volatile bool should_save = false;

volatile uint32_t start_time_us = 0;
volatile uint32_t last_time_us = 0;
volatile int32_t first_target_speed_cm_s = DEFAULT_SPEED_CM_S;
volatile int32_t current_speed_cm_s = 0;
volatile int32_t target_speed_cm_s = first_target_speed_cm_s;
volatile uint32_t self_enter_time_us;
volatile uint32_t self_exit_time_us;
volatile uint32_t other_enter_time_us = INVALID_TIME_US;
volatile uint32_t other_exit_time_us = INVALID_TIME_US;

volatile bool is_running = false;
volatile bool is_collision_detected = false;
volatile bool is_lonely = false;
volatile bool is_logging = false;


float pid_kp=PID_KP_DEFAULT;
float pid_ki=PID_KI_DEFAULT;
float pid_kd=PID_KD_DEFAULT;