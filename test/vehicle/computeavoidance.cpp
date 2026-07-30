#include "espnow.hpp"
#include "motor.hpp"
#include "sensor.hpp"
#include "callbacks.hpp"
#include "controller.hpp"

int computeAvoidanceSpeed(int OtherEnterTime,int OtherExitTime){
    int MyEnterTime = enterTime;
    int MyExitTime = exitTime;
    int NewTargetSpeed;
    uint32_t now = micros() - START_US;
    // MyEnterTime,MyExitTime,OtherEnterTime,OtherExitTime,nowは[μs],NewTargetSpeedは[cm/s]であることに注意

        // TODO

    return NewTargetSpeed;
}