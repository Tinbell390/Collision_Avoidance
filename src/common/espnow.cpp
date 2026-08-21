#include<WiFi.h>
#include<esp_now.h>
#include "espnow.hpp"
#include "callbacks.hpp"

const uint8_t broadcastAddress[6] ={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

//--------------------------------------------------
// ESPNOWセットアップ関数
//--------------------------------------------------
void setup_esp_now(){
    WiFi.mode(WIFI_STA);

    esp_now_init();

    esp_now_peer_info_t broadcast = {};
    memcpy(broadcast.peer_addr, broadcastAddress, 6);
    broadcast.channel = 0;
    broadcast.encrypt = false;

    esp_now_add_peer(&broadcast);

    esp_now_register_recv_cb(on_receive_data);
}

//--------------------------------------------------
// START送信関数
//--------------------------------------------------
void send_start(bool collision,bool lonely){
    StartPacket packet;

    packet.header.type = PacketType::START;
    packet.is_collision = collision;
    packet.is_lonely = lonely;

    esp_now_send(broadcastAddress,reinterpret_cast<uint8_t*>(&packet),sizeof(packet));
}

//--------------------------------------------------
// FINISH送信関数
//--------------------------------------------------
void send_finish(){
    FinishPacket packet;

    packet.header.type = PacketType::FINISH;

    esp_now_send(broadcastAddress,reinterpret_cast<uint8_t*>(&packet),sizeof(packet));
}

//--------------------------------------------------
// STATUS送信関数
//--------------------------------------------------
void send_status(const StatusData &data){
    StatusPacket packet;

    packet.header.type = PacketType::STATUS;
    packet.payload = data;

    esp_now_send(broadcastAddress,reinterpret_cast<uint8_t*>(&packet),sizeof(packet));
}

//--------------------------------------------------
// SETSPEED送信関数
//--------------------------------------------------
void send_speed(const int speed){
    SetSpeedPacket packet;
    packet.header.type=PacketType::SET_SPEED;
    packet.target_speed_cm_s = speed;
    esp_now_send(broadcastAddress,reinterpret_cast<uint8_t*>(&packet),sizeof(packet));
}

//--------------------------------------------------
// GAIN送信関数
//--------------------------------------------------
void send_gain(float kp,float ki,float kd){
    GainPacket packet;
    packet.header.type=PacketType::GAIN;
    packet.kp=kp;
    packet.ki=ki;
    packet.kd=kd;
    esp_now_send(broadcastAddress,reinterpret_cast<uint8_t*>(&packet),sizeof(packet));
}