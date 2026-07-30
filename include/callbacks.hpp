#ifndef CALLBACK_H
#define CALLBACK_H
#include "config.hpp"

void OnStartPacket(bool collision);
void OnFinishPacket();
void OnStatusPacket(const uint8_t *mac_addr,StatusData payload);
void OnSetSpeedPacket(int speed);
void OnGAINPacket(float kp,float ki,float kd);
#endif 