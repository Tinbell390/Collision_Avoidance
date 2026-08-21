#ifndef COLLISION_AVOIDANCE_H
#define COLLISION_AVOIDANCE_H

#include <stdint.h>

int32_t calculate_collision_avoidance_speed_cm_s(
    uint32_t other_enter_time_us,
    uint32_t other_exit_time_us
);

#endif // COLLISION_AVOIDANCE_H