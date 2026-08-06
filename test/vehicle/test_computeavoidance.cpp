#include "config.hpp"
#include "prediction.hpp"

//--------------------------------------------------
// 衝突回避速度算出関数
//--------------------------------------------------
int calculateCollisionAvoidanceSpeed(int OtherEnterTime,int OtherExitTime){
    int currentPosition = LineCount * LINE_PITCH;   // [mm]
    uint32_t MyEnterTime = (uint32_t)((uint64_t)(DIST_TO_INTERSECTION_ENTRY - currentPosition) * 1000000ULL / (CurrentSpeed * 10));
    uint32_t MyExitTime = (uint32_t)((uint64_t)(DIST_TO_INTERSECTION_EXIT + VEHICLE_LENGTH - currentPosition) * 1000000ULL / (CurrentSpeed * 10));
    int NewTargetSpeed = CurrentSpeed;
    uint32_t now = micros() - START_US;

        // TODO

    return NewTargetSpeed;
}

//--------------------------------------------------
// calculateCollisionAvoidanceSpeedを設計せよ
//
// MyEnterTime, MyExitTime,
// OtherEnterTime, OtherExitTime, nowは[μs]
// NewTargetSpeedは[cm/s]であることに注意すること。
//
// 自車両と他車両の交差点進入・退出予測時刻から
// 交差点内での衝突判定を行い、衝突を回避する目標速度を算出せよ。
//
// CollisionFlagが真の場合：
//   車両同士が衝突するように制御すること。
//
// CollisionFlagが偽の場合：
//   車両同士が衝突しないように制御すること。
//   自車両の交差点進入時刻を調整することで必要速度を計算し、返すこと．
//--------------------------------------------------

