#ifndef CONFIG_H
#define CONFIG_H
#include <Arduino.h>

constexpr uint32_t INVALID_TIME = UINT32_MAX;

//--------------------------------------------------
// ピン設定
//--------------------------------------------------
constexpr int PHASE_PIN = D5;
constexpr int ENABLE_PIN = D3;
constexpr int SENSOR_PIN = D6;
constexpr int RUN_SWITCH_PIN = D2;   

//--------------------------------------------------
// PWM
//--------------------------------------------------
constexpr int ledcChannel = 0;
constexpr int freq = 20000;
constexpr int resolution = 8;

//--------------------------------------------------
// パラメータ
//--------------------------------------------------
constexpr int INTERVAL_MS = 20;
constexpr int DEBOUNCE_US = 200;
constexpr int LINE_PITCH = 10;
constexpr int DIST_TO_INTERSECTION_ENTRY = 2000; // 交差点入口までの距離 [mm]
constexpr int DIST_TO_INTERSECTION_EXIT  = 200; // 交差点間の距離 [mm]
constexpr int VEHICLE_LENGTH             = 150; // 車体全長 [mm]
constexpr int basespeed = 100;                 // 初期速度[cm/s]

// メディアンフィルタパラメータ
constexpr int median_window_size = 7;

// PIDコントローラパラメータ
constexpr float F_KP = 1.0;
constexpr float F_KI = 1.0;
constexpr float F_KD = 0.0;

constexpr int MAX_INTEGRAL = 200;

constexpr int MAX_PWM = 200;
constexpr int MIN_PWM = 10;

constexpr int MAX_SPEED = 400;
constexpr int MIN_SPEED = 20;

constexpr int margin_us=50000;

// 車両数
constexpr int vehiclecount = 2;

// 推定到着時間を発信するための閾値
constexpr int speedborder = 20;

//--------------------------------------------------
// ログ構造体
//--------------------------------------------------
struct StatusData{//espnowで共有するデータ
    uint32_t time_us;
    uint32_t speed;
    uint8_t pwm;
    uint32_t enterTime;
    uint32_t exitTime;
};

//--------------------------------------------------
// extern
//--------------------------------------------------

extern volatile int SensorlogIndex;
extern volatile int LineCount;
extern volatile uint8_t CurrentPWM;

extern volatile bool StartFlag;
extern volatile bool SaveFlag;
extern volatile bool SensorONFlag;

extern volatile uint32_t START_US;
extern volatile uint32_t LAST_US;

extern volatile int CurrentSpeed;
extern volatile int targetSpeed;
extern volatile int FirstTargetSpeed;

extern volatile uint32_t enterTime;
extern volatile uint32_t exitTime;

extern volatile bool RunFlag;
extern volatile bool CollisionFlag;
extern volatile bool LoneryFlag;
extern volatile bool LogFlag;

extern float KP;
extern float KI;
extern float KD;




#endif // CONFIG_H