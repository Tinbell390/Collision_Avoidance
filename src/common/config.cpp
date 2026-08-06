#include "config.hpp"

volatile int SensorlogIndex = 0;
volatile int LineCount = 0;

volatile uint8_t CurrentPWM = 0;

volatile bool StartFlag = false;
volatile bool SaveFlag = false;
volatile bool SensorONFlag = false;

volatile uint32_t START_US = 0;
volatile uint32_t LAST_US = 0;
volatile int FirstTargetSpeed = basespeed;
volatile int CurrentSpeed = 0;
volatile int targetSpeed = FirstTargetSpeed;
volatile uint32_t enterTime;
volatile uint32_t exitTime;


volatile bool RunFlag = false;
volatile bool CollisionFlag = false;
volatile bool LoneryFlag = false;



float KP=F_KP;
float KI=F_KI;
float KD=F_KD;