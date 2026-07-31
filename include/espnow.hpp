#ifndef ESPNOW_HPP
#define ESPNOW_HPP

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
    bool lonelyFlag;
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

void setup_ESPNOW();

void SendSTART(bool collision,bool lonely);

void SendFINISH();

void SendSTATUS(const StatusData &data);

void SendSPEED(const int speed);

void SendGAIN(float kp,float ki,float kd);

#endif