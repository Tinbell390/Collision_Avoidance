#include "config.hpp"
#include "prediction.hpp"


//--------------------------------------------------
// 衝突回避速度算出関数
//--------------------------------------------------
int calculateCollisionAvoidanceSpeed(uint32_t OtherEnterTime,uint32_t OtherExitTime){
    int currentPosition = LineCount * LINE_PITCH;   // [mm]
    uint32_t MyEnterTime = (uint32_t)((uint64_t)(DIST_TO_INTERSECTION_ENTRY - currentPosition) * 1000000ULL / (CurrentSpeed * 10));
    uint32_t MyExitTime = (uint32_t)((uint64_t)(DIST_TO_INTERSECTION_EXIT + VEHICLE_LENGTH - currentPosition) * 1000000ULL / (CurrentSpeed * 10));
    int NewTargetSpeed = CurrentSpeed; ;
    uint32_t now = micros() - START_US;
    /////////////// 
    uint32_t desiredEnter;
    if (CollisionFlag){
        // 相手と同時に突入
        desiredEnter = OtherEnterTime;
    }
    else if (OtherEnterTime>MyEnterTime){
        //自分が相手より早く突入
        desiredEnter = OtherEnterTime - margin_us - (MyExitTime - MyEnterTime);
    }
    else{
        //自分が相手より遅く突入
        desiredEnter = OtherExitTime + margin_us; 
    }

    if (desiredEnter <= now) return targetSpeed;

    int remain = DIST_TO_INTERSECTION_ENTRY - LineCount * LINE_PITCH;

    if (remain <= 0) return targetSpeed;

    uint32_t remainTime = desiredEnter - now;

    NewTargetSpeed = (int)constrain(remain * 1000000UL / remainTime,MIN_SPEED,MAX_SPEED);
    ////////////////
    return NewTargetSpeed;
}