#include <Arduino.h>
#include "config.hpp"
#include "espnow.hpp"
#include "motor.hpp"
#include "sensor.hpp"
#include "controller.hpp"
#include "prediction.hpp"

// 実験2,実験3で使用する車両用

StatusData CurrentSTATUS;

void setup(){
    Serial.begin(115200);
    setup_ESPNOW();
    setup_motor();
    setup_sensor();

}

// メインループ
void loop()
{
    if(RunFlag){
        CurrentSpeed=getCurrentSpeed();
        CurrentPWM = PIDcontroller(CurrentSpeed, targetSpeed);
        motorWrite(CurrentPWM);
        CurrentSTATUS.time_us=micros() - START_US;
        CurrentSTATUS.speed = CurrentSpeed;
        CurrentSTATUS.pwm=CurrentPWM; 
        enterTime=predictTimeToIntersection();
        exitTime=predictTimeToExitIntersection();     
        CurrentSTATUS.enterTime=enterTime;
        CurrentSTATUS.exitTime=exitTime;
        SendSTATUS(CurrentSTATUS);

        // 交差点を通過したら停止
        if(LineCount>DIST_TO_INTERSECTION_ENTRY/LINE_PITCH){
            RunFlag=false;
            motorBrake();
        }
    }
    else{
        motorBrake();
    }
    delay(INTERVAL_MS);
}