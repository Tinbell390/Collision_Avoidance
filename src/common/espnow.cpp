#include "espnow.hpp"
#include "logger.hpp"
#include "motor.hpp"
#include "sensor.hpp"
#include "callbacks.hpp"

const uint8_t broadcastAddress[6] ={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

//--------------------------------------------------
// ESPNOWセットアップ関数
//--------------------------------------------------
void setup_ESPNOW(){
    WiFi.mode(WIFI_STA);

    esp_now_init();

    esp_now_peer_info_t broadcast = {};
    memcpy(broadcast.peer_addr, broadcastAddress, 6);
    broadcast.channel = 0;
    broadcast.encrypt = false;

    esp_now_add_peer(&broadcast);

    esp_now_register_recv_cb(OnRecvData);
}

//--------------------------------------------------
// START送信関数
//--------------------------------------------------
void SendSTART(bool collision){
    STARTPacket packet;

    packet.head.type = PacketType::START;
    packet.CollisionFlag = collision;

    esp_now_send(broadcastAddress,reinterpret_cast<uint8_t*>(&packet),sizeof(packet));
}

//--------------------------------------------------
// FINISH送信関数
//--------------------------------------------------
void SendFINISH(){
    FINISHPacket packet;

    packet.head.type = PacketType::FINISH;

    esp_now_send(broadcastAddress,reinterpret_cast<uint8_t*>(&packet),sizeof(packet));
}

//--------------------------------------------------
// STATUS送信関数
//--------------------------------------------------
void SendSTATUS(const StatusData &data){
    STATUSPacket packet;

    packet.head.type = PacketType::STATUS;
    packet.payload = data;

    esp_now_send(broadcastAddress,reinterpret_cast<uint8_t*>(&packet),sizeof(packet));
}

//--------------------------------------------------
// SETSPEED送信関数
//--------------------------------------------------
void SendSPEED(const int speed){
    SETSPEEDPacket packet;
    packet.head.type=PacketType::SET_SPEED;
    packet.targetSpeed = speed;
    esp_now_send(broadcastAddress,reinterpret_cast<uint8_t*>(&packet),sizeof(packet));
}

//--------------------------------------------------
// GAIN送信関数
//--------------------------------------------------
void SendGAIN(float kp,float ki,float kd){
    GAINPacket packet;
    packet.head.type=PacketType::GAIN;
    packet.KP=kp;
    packet.KI=ki;
    packet.KD=kd;
    esp_now_send(broadcastAddress,reinterpret_cast<uint8_t*>(&packet),sizeof(packet));
}