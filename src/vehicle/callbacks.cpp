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

//--------------------------------------------------
// ビークル受信コールバック関数
//--------------------------------------------------
void OnRecvData(const uint8_t *mac_addr,const uint8_t *data,int len){
    const PacketHead *head=reinterpret_cast<const PacketHead *>(data);

    switch (head->type){
        case PacketType::START:{
            const STARTPacket *packet=reinterpret_cast<const STARTPacket *>(data);
            OnStartPacket(packet->CollisionFlag);
            break;
        }
        case PacketType::FINISH:{
            OnFinishPacket();
            break;
        }

        case PacketType::STATUS:{
            STATUSPacket packet;
            memcpy(&packet, data, sizeof(packet));  
            StatusData payload=packet.payload;
            OnStatusPacket(mac_addr,payload);
            break;
        }

        case PacketType::SET_SPEED:{
            const SETSPEEDPacket *packet=reinterpret_cast<const SETSPEEDPacket *>(data);
            int speed = packet->targetSpeed;
            OnSetSpeedPacket(speed);
            break;
        }
        case PacketType::GAIN:{
            const GAINPacket *packet=reinterpret_cast<const GAINPacket *>(data);
            float kp=packet->KP;
            float ki=packet->KI;
            float kd=packet->KD;
            OnGAINPacket(kp,ki,kd);
            break;

        }
    }
}