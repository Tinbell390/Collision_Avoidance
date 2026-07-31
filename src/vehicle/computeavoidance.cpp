#include "config.hpp"
#include "prediction.hpp"


//--------------------------------------------------
// 衝突回避速度算出関数
//--------------------------------------------------
int calculateCollisionAvoidanceSpeed(uint32_t OtherEnterTime,uint32_t OtherExitTime){
    uint32_t MyEnterTime = predictTimeToIntersection();
    uint32_t MyExitTime = predictTimeToExitIntersection();
    int NewTargetSpeed ;
    uint32_t now = micros() - START_US;
    /////////////// 
    uint32_t desiredEnter;
    if (CollisionFlag){
        // 相手と同時に突入
        desiredEnter = OtherEnterTime;
    }
    else if (OtherEnterTime>MyEnterTime){
        //自分が相手より早く突入
        desiredEnter = OtherEnterTime - margin_us - (exitTime - enterTime);
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