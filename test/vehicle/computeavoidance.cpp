#include "config.hpp"
#include "prediction.hpp"

//--------------------------------------------------
// 衝突回避速度算出関数
//--------------------------------------------------
int calculateCollisionAvoidanceSpeed(int OtherEnterTime,int OtherExitTime){
    uint32_t MyEnterTime = predictTimeToIntersection();
    uint32_t MyExitTime = predictTimeToExitIntersection();
    int NewTargetSpeed;
    uint32_t now = micros() - START_US;

        // TODO

    return NewTargetSpeed;
}

// calculateCollisionAvoidanceSpeedを設計せよ
// MyEnterTime,MyExitTime,OtherEnterTime,OtherExitTime,nowは[μs],NewTargetSpeedは[cm/s]であることに注意
// CollisionFlagが真なら車両同士が衝突するように制御
// CollisionFlagが偽なら衝突回避制御

