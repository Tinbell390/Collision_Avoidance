#include "espnow.hpp"
#include "logger.hpp"
#include "motor.hpp"
#include "sensor.hpp"
#include "callbacks.hpp"


// パケット種類
//==================================================
enum class PacketType : uint8_t{
    START = 0,
    FINISH,
    STATUS,
    SET_SPEED,
    GAIN
};

// パケット
//==================================================
struct PacketHead{
    PacketType type;
};

struct STARTPacket{
    PacketHead head;
    bool CollisionFlag;
};

struct FINISHPacket{
    PacketHead head;
};

struct STATUSPacket{
    PacketHead head;

    StatusData payload;
};

struct SETSPEEDPacket{
    PacketHead head;
    uint16_t targetSpeed;   
};

struct GAINPacket{
    PacketHead head;
    float KP;
    float KI;
    float KD;
};

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
}

//--------------------------------------------------
// モニター受信コールバック関数
//--------------------------------------------------
void Recv_Monitor(const uint8_t *mac_addr,const uint8_t *data,int len){
    const PacketHead *head=reinterpret_cast<const PacketHead *>(data);

    if (head->type != PacketType::STATUS) return;

    STATUSPacket packet;
    memcpy(&packet, data, sizeof(packet));  
    StatusData payload=packet.payload;

    OnStatusPacket(mac_addr,payload);
}

//-------------------------------------------------
// モニター受信コールバックセット
//--------------------------------------------------
void set_Monitor(){
    esp_now_register_recv_cb(Recv_Monitor);
}

//--------------------------------------------------
// ビークル受信コールバック関数
//--------------------------------------------------
void Recv_Vehicle(const uint8_t *mac_addr,const uint8_t *data,int len){
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

//--------------------------------------------------
// モニター受信コールバックセット
//--------------------------------------------------
void set_Vehicle(){
    esp_now_register_recv_cb(Recv_Vehicle);
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