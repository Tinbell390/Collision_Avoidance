#include "espnow.hpp"
#include "motor.hpp"
#include "sensor.hpp"
#include "callbacks.hpp"
#include "controller.hpp"
//ビークル側

//--------------------------------------------------
// STARTパケット受信処理関数
//--------------------------------------------------
void OnStartPacket(bool collision){
    START_US=micros();
    RunFlag = true;
    CollisionFlag = collision;
    ClearController();
    clearIntervalBuffer();
}

//--------------------------------------------------
// FINISHパケット受信処理関数
//--------------------------------------------------
void OnFinishPacket(){
    RunFlag = false;
    return;
}

//--------------------------------------------------
// STATUSパケット受信処理関数
//--------------------------------------------------
void OnStatusPacket(const uint8_t *mac_addr,StatusData payload){

    if (payload.enterTime == INVALID_TIME || payload.exitTime  == INVALID_TIME){
        return;
    }
    uint32_t desiredEnter;

    if (CollisionFlag){
        // 相手と同時に突入
        desiredEnter = payload.enterTime;
    }
    else if (payload.enterTime>enterTime){
        //自分が相手より早く突入
        desiredEnter = enterTime - margin_us;       //50ms早く突入
    }
    else{
        //自分が相手より遅く突入
        desiredEnter = payload.exitTime + margin_us; // 50ms遅く突入
    }

        uint32_t now = micros() - START_US;

    if (desiredEnter <= now) return;

    int remain = DIST_TO_INTERSECTION_ENTRY - LineCount * LINE_PITCH;

    if (remain <= 0) return;

    uint32_t remainTime = desiredEnter - now;

    targetSpeed = constrain(remain * 1000000UL / remainTime,MIN_SPEED,MAX_SPEED);

    return;
}

//--------------------------------------------------
// SETSPEEDパケット受信処理関数
//--------------------------------------------------
void OnSetSpeedPacket(int speed){
    targetSpeed=speed;
    return;
}

//--------------------------------------------------
// GAINパケット受信処理関数
//--------------------------------------------------
void OnGAINPacket(float kp,float ki,float kd){
    KP = kp;
    KI = ki;
    KD = kd;
    return ;
}