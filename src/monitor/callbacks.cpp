#include "config.hpp"
#include "espnow.hpp"
#include "logger.hpp"
#include "callbacks.hpp"
//モニター側
uint8_t vehicleaddress[vehiclecount][6];
bool addressRegistered[vehiclecount] = {false};
//--------------------------------------------------
// STARTパケット受信処理関数
//--------------------------------------------------
void OnStartPacket(bool collision){
    return;
}

//--------------------------------------------------
// FINISHパケット受信処理関数
//--------------------------------------------------
void OnFinishPacket(){
    return;
}

//--------------------------------------------------
// 車両番号取得（未登録なら登録）
//--------------------------------------------------
int GetVehicleIndex(const uint8_t *mac_addr){

    // 既登録か検索
    for(int i = 0; i < vehiclecount; i++){

        if(addressRegistered[i] &&memcmp(vehicleaddress[i], mac_addr, 6) == 0){
            return i;
        }
    }

    // 未登録なら空きを探す
    for(int i = 0; i < vehiclecount; i++){

        if(!addressRegistered[i]){

            memcpy(vehicleaddress[i], mac_addr, 6);
            addressRegistered[i] = true;

            Serial.printf("Vehicle %d registered\n", i);

            return i;
        }
    }

    // 登録できない
    return -1;
}

void OnStatusPacket(const uint8_t *mac_addr, StatusData payload){
    if(LogFlag) return;   // ログ転送中は受信を無視
    int index = GetVehicleIndex(mac_addr);

    if(index < 0){
        Serial.println("Vehicle table full.");
        return;
    }

    Serial.println("----------------------------------------");
    Serial.printf("Vehicle     : %d\n", index);
    Serial.printf("MAC Address : %02X:%02X:%02X:%02X:%02X:%02X\n",
                  mac_addr[0], mac_addr[1], mac_addr[2],
                  mac_addr[3], mac_addr[4], mac_addr[5]);

    Serial.printf("Time  : %lu\n", payload.time_us);
    Serial.printf("Speed : %d\n", payload.speed);
    Serial.printf("PWM   : %d\n", payload.pwm);
    Serial.println("----------------------------------------");

    WriteLog(index, payload);
}

//--------------------------------------------------
// SETSPEEDパケット受信処理関数
//--------------------------------------------------
void OnSetSpeedPacket(int speed){
    return;
}

//--------------------------------------------------
// GAINパケット受信処理関数
//--------------------------------------------------
void OnGAINPacket(float kp,float ki,float kd){
    return ;
}

//--------------------------------------------------
// モニター受信コールバック関数
//--------------------------------------------------
void OnRecvData(const uint8_t *mac_addr,const uint8_t *data,int len){
    const PacketHead *head=reinterpret_cast<const PacketHead *>(data);

    if(head->type != PacketType::STATUS) return;

    STATUSPacket packet;
    memcpy(&packet, data, sizeof(packet));  
    StatusData payload=packet.payload;

    OnStatusPacket(mac_addr,payload);
}