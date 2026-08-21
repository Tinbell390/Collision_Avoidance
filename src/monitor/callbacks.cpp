#include "config.hpp"
#include "espnow.hpp"
#include "logger.hpp"
#include "callbacks.hpp"
//モニター側
uint8_t vehicle_mac_addresses[VEHICLE_COUNT][6];
bool is_vehicle_registered[VEHICLE_COUNT] = {false};
//--------------------------------------------------
// STARTパケット受信処理関数
//--------------------------------------------------
void handle_start_packet(bool collision){
    return;
}

//--------------------------------------------------
// FINISHパケット受信処理関数
//--------------------------------------------------
void handle_finish_packet(){
    return;
}

//--------------------------------------------------
// 車両番号取得（未登録なら登録）
//--------------------------------------------------
int32_t get_vehicle_index(const uint8_t *mac_addr){

    // 既登録か検索
    for(int i = 0; i < VEHICLE_COUNT; i++){

        if(is_vehicle_registered[i] &&memcmp(vehicle_mac_addresses[i], mac_addr, 6) == 0){
            return i;
        }
    }

    // 未登録なら空きを探す
    for(int i = 0; i < VEHICLE_COUNT; i++){

        if(!is_vehicle_registered[i]){

            memcpy(vehicle_mac_addresses[i], mac_addr, 6);
            is_vehicle_registered[i] = true;

            Serial.printf("Vehicle %d registered\n", i);

            return i;
        }
    }

    // 登録できない
    return -1;
}

void handle_status_packet(const uint8_t *mac_addr, const StatusData payload){
    if(is_logging) return;   // ログ転送中は受信を無視
    int32_t index = get_vehicle_index(mac_addr);

    if(index < 0){
        Serial.println("Vehicle table full.");
        return;
    }

    Serial.println("----------------------------------------");
    Serial.printf("Vehicle     : %d\n", index);
    Serial.printf("MAC Address : %02X:%02X:%02X:%02X:%02X:%02X\n",
                  mac_addr[0], mac_addr[1], mac_addr[2],
                  mac_addr[3], mac_addr[4], mac_addr[5]);

    Serial.printf("Time  : %lu\n", payload.timestamp_us);
    Serial.printf("Speed : %d\n", payload.speed_cm_s);
    Serial.printf("PWM   : %d\n", payload.pwm);
    Serial.println("----------------------------------------");

    write_log(index, payload);
}

//--------------------------------------------------
// SETSPEEDパケット受信処理関数
//--------------------------------------------------
void handle_set_speed_packet(int speed){
    return;
}

//--------------------------------------------------
// GAINパケット受信処理関数
//--------------------------------------------------
void handle_gain_packet(float kp,float ki,float kd){
    return ;
}

//--------------------------------------------------
// モニター受信コールバック関数
//--------------------------------------------------
void on_receive_data(const uint8_t *mac_addr,const uint8_t *data,int len){
    const PacketHead *header=reinterpret_cast<const PacketHead *>(data);

    if(header->type != PacketType::STATUS) return;

    StatusPacket packet;
    memcpy(&packet, data, sizeof(packet));  
    StatusData payload=packet.payload;

    handle_status_packet(mac_addr,payload);
}