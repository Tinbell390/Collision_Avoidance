#ifndef ESPNOW_HPP
#define ESPNOW_HPP
#include"config.hpp"

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

struct StartPacket{
    PacketHead header;
    bool is_collision;
    bool is_lonely;
};

struct FinishPacket{
    PacketHead header;
};

struct StatusPacket{
    PacketHead header;

    StatusData payload;
};

struct SetSpeedPacket{
    PacketHead header;
    uint32_t target_speed_cm_s;   
};

struct GainPacket{
    PacketHead header;
    float kp;
    float ki;
    float kd;
};

void setup_esp_now();

void send_start(bool collision,bool lonely);

void send_finish();

void send_status(const StatusData &data);

void send_speed(const int speed);

void send_gain(float kp,float ki,float kd);

#endif