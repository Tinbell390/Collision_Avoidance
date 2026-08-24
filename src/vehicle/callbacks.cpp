#include "espnow.hpp"
#include "motor.hpp"
#include "sensor.hpp"
#include "callbacks.hpp"
#include "controller.hpp"

//ビークル側

//--------------------------------------------------
// STARTパケット受信処理関数
//--------------------------------------------------
void handle_start_packet(bool collision,bool lonery){
    start_time_us=micros();
    last_time_us=start_time_us;
    is_running = true;
    is_collision_detected = collision;
    is_lonely = lonery;
    target_speed_cm_s=first_target_speed_cm_s;
    line_count=0;
    reset_controller();
    clear_sensor_interval_buffer();
}

//--------------------------------------------------
// FINISHパケット受信処理関数
//--------------------------------------------------
void handle_finish_packet(){
    is_running = false;
    return;
}

//--------------------------------------------------
// STATUSパケット受信処理関数
//--------------------------------------------------
void handle_status_packet(const uint8_t *mac_addr, const StatusData payload){

    if (payload.time_to_enter_intersection_us == INVALID_TIME_US || payload.time_to_exit_intersection_us  == INVALID_TIME_US || is_lonely){
        return;
    }
    other_enter_time_us = payload.time_to_enter_intersection_us;
    other_exit_time_us = payload.time_to_exit_intersection_us;
    return;
}

//--------------------------------------------------
// SETSPEEDパケット受信処理関数
//--------------------------------------------------
void handle_set_speed_packet(int speed){
    target_speed_cm_s=speed;
    first_target_speed_cm_s = speed;
    return;
}

//--------------------------------------------------
// GAINパケット受信処理関数
//--------------------------------------------------
void handle_gain_packet(float kp,float ki,float kd){
    pid_kp = kp;
    pid_ki = ki;
    pid_kd = kd;
    return ;
}

//--------------------------------------------------
// ビークル受信コールバック関数
//--------------------------------------------------
void on_receive_data(const uint8_t *mac_addr,const uint8_t *data,int len){
    const PacketHead *header=reinterpret_cast<const PacketHead *>(data);

    switch (header->type){
        case PacketType::START:{
            const StartPacket *packet=reinterpret_cast<const StartPacket *>(data);
            handle_start_packet(packet->is_collision,packet->is_lonely);
            break;
        }
        case PacketType::FINISH:{
            handle_finish_packet();
            break;
        }

        case PacketType::STATUS:{
            StatusPacket packet;
            memcpy(&packet, data, sizeof(packet));  
            StatusData payload=packet.payload;
            handle_status_packet(mac_addr,payload);
            break;
        }

        case PacketType::SET_SPEED:{
            const SetSpeedPacket *packet=reinterpret_cast<const SetSpeedPacket *>(data);
            int speed = packet->target_speed_cm_s;
            handle_set_speed_packet(speed);
            break;
        }
        case PacketType::GAIN:{
            const GainPacket *packet=reinterpret_cast<const GainPacket *>(data);
            float kp=packet->kp;
            float ki=packet->ki;
            float kd=packet->kd;
            handle_gain_packet(kp,ki,kd);
            break;

        }
    }
}