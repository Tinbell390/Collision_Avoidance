#include "espnow.hpp"
#include "motor.hpp"
#include "sensor.hpp"
#include "callbacks.hpp"
#include "controller.hpp"

//--------------------------------------------------
// 衝突回避速度算出関数
//--------------------------------------------------
int computeAvoidanceSpeed(int OtherEnterTime,int OtherExitTime){
    int MyEnterTime = enterTime;
    int MyExitTime = exitTime;
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
        desiredEnter = enterTime - margin_us;       //50ms早く突入
    }
    else{
        //自分が相手より遅く突入
        desiredEnter = OtherExitTime + margin_us; // 50ms遅く突入
    }

    if (desiredEnter <= now) return targetSpeed;

    int remain = DIST_TO_INTERSECTION_ENTRY - LineCount * LINE_PITCH;

    if (remain <= 0) return targetSpeed;

    uint32_t remainTime = desiredEnter - now;

    NewTargetSpeed = constrain(remain * 1000000UL / remainTime,MIN_SPEED,MAX_SPEED);
    ////////////////
    return NewTargetSpeed;
}