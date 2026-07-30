#include <Arduino.h>
#include "config.hpp"
#include "sensor.hpp"

uint32_t intervalBuffer[median_window_size];

int head = 0;
int count = 0;

//--------------------------------------------------
// センサログ追加関数
//--------------------------------------------------
void pushInterval(uint32_t interval){
    intervalBuffer[head] = interval;

    head = (head + 1) % median_window_size;

    if (count < median_window_size){
        count++;
    }
}

//--------------------------------------------------
// センサログクリア関数
//--------------------------------------------------
void clearIntervalBuffer(){
    head = 0;
    count = 0;
}

//--------------------------------------------------
// メディアンフィルタ関数
//--------------------------------------------------
uint32_t medianInterval(){
    if (count == 0){
        return 100000000;
    }

    uint32_t work[median_window_size];

    for (int i = 0; i < count; i++){
        work[i] = intervalBuffer[i];
    }

    for (int i = 0; i < count - 1; i++){
        for (int j = i + 1; j < count; j++){
            if (work[i] > work[j]){
                uint32_t tmp = work[i];
                work[i] = work[j];
                work[j] = tmp;
            }
        }
    }

    return work[count / 2];
}

//--------------------------------------------------
// 現在速度取得関数
//--------------------------------------------------
int getCurrentSpeed(){
    int speed;
    uint32_t median_interval = medianInterval();
    uint32_t now_interval = micros() - LAST_US;
    uint32_t true_interval = max(median_interval,now_interval);
    speed = LINE_PITCH*100000/true_interval;
    return speed;
}

//--------------------------------------------------
// ライン検出関数
//--------------------------------------------------
void IRAM_ATTR sensorISR(){
    bool state = digitalRead(SENSOR_PIN);

    //入力がHIGHでセンサーONフラグがfalseのとき記録をとる
    if(state&&!SensorONFlag){
        //現在時間を取得
        uint32_t NOW_US = micros();
        uint32_t Interval_US = NOW_US - LAST_US;

        //最新時間からデバウンス時間経過していないなら無視
        if (Interval_US>DEBOUNCE_US){
            //ライン検出をインクリメント
            LineCount++;
            //ログデータを配列に格納
            pushInterval(Interval_US);

            //最新時間を更新
            LAST_US=NOW_US;
            //インデックスを更新
            SensorlogIndex++;
            SensorONFlag = true;
        }
    }
    //入力がLOWならセンサーONフラグをfalseにする
    else if(!state){
        SensorONFlag = false;
    }
}
//--------------------------------------------------
// センサセットアップ
//--------------------------------------------------
void setup_sensor(){
    //ピンをセット
    pinMode(SENSOR_PIN, INPUT);

    // ライン検出関数の割り込みをセット
    attachInterrupt(digitalPinToInterrupt(SENSOR_PIN),sensorISR,CHANGE);
}