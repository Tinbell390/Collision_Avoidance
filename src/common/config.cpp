#include "config.hpp"

volatile int SensorlogIndex = 0;
volatile int LineCount = 0;

volatile uint8_t CurrentPWM = 0;

volatile bool StartFlag = false;
volatile bool SaveFlag = false;
volatile bool SensorONFlag = false;

uint32_t START_US = 0;
uint32_t LAST_US = 0;

int CurrentSpeed = 0;
int CurrentPosition = 0;
int targetSpeed = FirstTargetSpeed;
uint32_t enterTime;
uint32_t exitTime;


volatile bool RunFlag = false;
bool CollisionFlag = false;

int FirstTargetSpeed = 100;

float KP=F_KP;
float KI=F_KI;
float KD=F_KD;