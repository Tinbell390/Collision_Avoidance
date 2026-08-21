#include <Arduino.h>
#include "config.hpp"
constexpr uint64_t MICROSECONDS_PER_SECOND = 1000000ULL;
constexpr int32_t MM_PER_CM = 10;
//--------------------------------------------------
// 交差点突入時間計算関数
//--------------------------------------------------
uint32_t predict_time_to_intersection_us() {

    int32_t current_position_mm = line_count * LINE_PITCH_MM;   // [mm]
    const int32_t speed_error_cm_s = target_speed_cm_s - current_speed_cm_s;
    // 停止中または速度が閾値を超えると計算不可
    if (current_speed_cm_s <= 0 || abs(speed_error_cm_s) > SPEED_THRESHOLD_CM_S) {
        return INVALID_TIME_US;
    }

    // 交差点入口までの残り距離 [mm]
    int32_t remaining_distance_mm = DIST_TO_INTERSECTION_ENTRY_MM - current_position_mm;

    // すでに到達済み
    if (remaining_distance_mm <= 0){
        return 0;
    }

    // [us]
    return static_cast<uint32_t>(static_cast<uint64_t>(remaining_distance_mm) * MICROSECONDS_PER_SECOND / (current_speed_cm_s * MM_PER_CM)
    );
}

uint32_t predict_time_to_exit_intersection_us(){

    int32_t current_position_mm = line_count * LINE_PITCH_MM;   // [mm]
    const int32_t speed_error_cm_s = target_speed_cm_s - current_speed_cm_s;
    // 停止中または速度が閾値を超えると計算不可
    if (current_speed_cm_s <= 0 || abs(speed_error_cm_s) > SPEED_THRESHOLD_CM_S) {
        return INVALID_TIME_US;
    }

    // 車体後端が交差点を抜けるまでの残り距離 [mm]
    int32_t remaining_distance_mm = DIST_TO_INTERSECTION_ENTRY_MM + DIST_TO_INTERSECTION_EXIT_MM + VEHICLE_LENGTH_MM - current_position_mm;

    // 既に通過済み
    if (remaining_distance_mm <= 0){
        return 0;
    }

    // [us]
    return static_cast<uint32_t>(static_cast<uint64_t>(remaining_distance_mm) * MICROSECONDS_PER_SECOND / (current_speed_cm_s * MM_PER_CM)
    );
}