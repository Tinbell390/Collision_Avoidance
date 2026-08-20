#include <Arduino.h>
#include "config.hpp"

//--------------------------------------------------
// 交差点突入時間計算関数
//--------------------------------------------------
uint32_t predictTimeToIntersection() {

    int currentPosition = LineCount * LINE_PITCH;   // [mm]
    int speed = CurrentSpeed;                       // [cm/s]

    // 停止中は計算不可
    if (speed <= 0||abs(targetSpeed-CurrentSpeed)>speedborder){
        return UINT32_MAX;
    }

    // 交差点入口までの残り距離 [mm]
    int remainingDistance = DIST_TO_INTERSECTION_ENTRY - currentPosition;

    // すでに到達済み
    if (remainingDistance <= 0){
        return 0;
    }

    // [us]
    return (uint32_t)((uint64_t)remainingDistance * 1000000ULL / (speed * 10));
}

uint32_t predictTimeToExitIntersection(){

    int currentPosition = LineCount * LINE_PITCH;   // [mm]
    int speed = CurrentSpeed;                       // [cm/s]

    // 停止中は計算不可
    if (speed <= 0||abs(targetSpeed-CurrentSpeed)>speedborder){
        return UINT32_MAX;
    }

    // 車体後端が交差点を抜けるまでの残り距離 [mm]
    int remainingDistance =
        DIST_TO_INTERSECTION_ENTRY + DIST_TO_INTERSECTION_EXIT + VEHICLE_LENGTH - currentPosition;

    // 既に通過済み
    if (remainingDistance <= 0){
        return 0;
    }

    // 時間[us]
    return (uint32_t)((uint64_t)remainingDistance * 1000000ULL / (speed * 10));
}