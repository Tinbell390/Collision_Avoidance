#include "espnow.hpp"
#include "motor.hpp"
#include "sensor.hpp"
#include "callbacks.hpp"
#include "controller.hpp"
#include "computavoidance.hpp"
//ビークル側

//--------------------------------------------------
// STARTパケット受信処理関数
//--------------------------------------------------
void OnStartPacket(bool collision){
    START_US=micros();
    RunFlag = true;
    CollisionFlag = collision;
    targetSpeed=FirstTargetSpeed;
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

    targetSpeed = computeAvoidanceSpeed(payload.enterTime,payload.exitTime);

    return;
}

//--------------------------------------------------
// SETSPEEDパケット受信処理関数
//--------------------------------------------------
void OnSetSpeedPacket(int speed){
    targetSpeed=speed;
    FirstTargetSpeed = speed;
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