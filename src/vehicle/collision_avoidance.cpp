#include "config.hpp"
#include "prediction.hpp"
#include "collision_avoidance.hpp"

constexpr uint64_t MICROSECONDS_PER_SECOND = 1000000ULL;
constexpr int32_t MM_PER_CM = 10;

//--------------------------------------------------
// 衝突回避速度算出関数
//--------------------------------------------------
int32_t calculate_collision_avoidance_speed_cm_s(uint32_t other_enter_time_us,uint32_t other_exit_time_us){
    int current_position_mm = line_count * LINE_PITCH_MM;   // [mm]
    uint32_t my_enter_time_us = static_cast<uint32_t>(static_cast<uint64_t>(DIST_TO_INTERSECTION_ENTRY_MM - current_position_mm) * MICROSECONDS_PER_SECOND / (current_speed_cm_s * MM_PER_CM));

    uint32_t my_exit_time_us = static_cast<uint32_t>(static_cast<uint64_t>(DIST_TO_INTERSECTION_EXIT_MM + VEHICLE_LENGTH_MM - current_position_mm) * MICROSECONDS_PER_SECOND / (current_speed_cm_s * MM_PER_CM));
    
    int32_t new_target_speed_cm_s = current_speed_cm_s;
    uint32_t current_time_us = micros() - start_time_us;
    /////////////// 
    uint32_t desired_enter_time_us;
    if (is_collision_detected){
        // 相手と同時に突入
        desired_enter_time_us = other_enter_time_us;
    }
    else if (other_enter_time_us>my_enter_time_us){
        //自分が相手より早く突入
        desired_enter_time_us = other_enter_time_us - TIME_MARGIN_US - (my_exit_time_us - my_enter_time_us);
    }
    else{
        //自分が相手より遅く突入
        desired_enter_time_us = other_exit_time_us + TIME_MARGIN_US; 
    }

    if (desired_enter_time_us <= current_time_us) return target_speed_cm_s;

    int remaining_distance_mm = DIST_TO_INTERSECTION_ENTRY_MM - line_count * LINE_PITCH_MM;

    if (remaining_distance_mm <= 0) return target_speed_cm_s;

    uint32_t remaining_time_us = desired_enter_time_us - current_time_us;

    new_target_speed_cm_s = static_cast<uint32_t>(constrain(remaining_distance_mm * MICROSECONDS_PER_SECOND / remaining_time_us,MIN_SPEED_CM_S,MAX_SPEED_CM_S));
    ////////////////
    return new_target_speed_cm_s;
}