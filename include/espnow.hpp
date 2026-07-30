#ifndef ESPNOW_HPP
#define ESPNOW_HPP

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "config.hpp"

void setup_ESPNOW();

void set_Monitor();

void set_Vehicle();

void SendSTART(bool collision);

void SendFINISH();

void SendSTATUS(const StatusData &data);

void SendSPEED(const int speed);

void SendGAIN(float kp,float ki,float kd);

#endif