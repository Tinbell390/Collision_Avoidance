#include "config.hpp"
#include "prediction.hpp"

//--------------------------------------------------
// 衝突回避速度算出関数
//--------------------------------------------------
int calculateCollisionAvoidanceSpeed(int OtherEnterTime,int OtherExitTime){
    int currentPosition = LineCount * LINE_PITCH;   // [mm]
    uint32_t MyEnterTime = (uint32_t)((uint64_t)(DIST_TO_INTERSECTION_ENTRY - currentPosition) * 1000000ULL / (CurrentSpeed * 10));
    uint32_t MyExitTime = (uint32_t)((uint64_t)(DIST_TO_INTERSECTION_EXIT + VEHICLE_LENGTH - currentPosition) * 1000000ULL / (CurrentSpeed * 10));
    int NewTargetSpeed ;
    uint32_t now = micros() - START_US;

        // TODO

    return NewTargetSpeed;
}

// calculateCollisionAvoidanceSpeedを設計せよ
// MyEnterTime,MyExitTime,OtherEnterTime,OtherExitTime,nowは[μs],NewTargetSpeedは[cm/s]であることに注意
// CollisionFlagが真なら車両同士が衝突するように制御
// CollisionFlagが偽なら衝突回避制御
// MyEnterTime,MyExitTimeの値がUINT32_MAXなら停止中or加速中であることに留意(相手の入退出時間しか使えない)

