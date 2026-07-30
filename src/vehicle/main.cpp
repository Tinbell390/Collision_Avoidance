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
    set_Vehicle();
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
        enterTime=predictTimeToIntersection();
        exitTime=predictTimeToExitIntersection();
        CurrentSTATUS.time_us=micros() - START_US;
        CurrentSTATUS.speed = CurrentSpeed;
        CurrentSTATUS.pwm=CurrentPWM;
        CurrentSTATUS.enterTime=enterTime;
        CurrentSTATUS.exitTime=exitTime;
        SendSTATUS(CurrentSTATUS);
    }
    else{
        motorBrake();
    }
    delay(INTERVAL_MS);
}