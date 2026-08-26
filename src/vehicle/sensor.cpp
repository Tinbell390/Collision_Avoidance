#include <Arduino.h>
#include "driver/gpio.h"
#include "esp_intr_alloc.h"
#include "config.hpp"
#include "sensor.hpp"
uint32_t interval_ring_buffer_us[FILTER_WINDOW_SIZE];    // センサデータを格納するリングバッファ

 volatile bool is_sensor_enabled = false;                         // センサの立ち上がり立ち下がりを判別するフラグ

int32_t interval_ring_buffer_head = 0;                  // リングバッファの次の書き込み場所
int32_t interval_ring_buffer_count = 0;                 // リングバッファに格納されたデータの数



//--------------------------------------------------
// センサ間隔データ追加
//--------------------------------------------------

void push_sensor_interval_us(uint32_t interval_us)
{
    interval_ring_buffer_us[interval_ring_buffer_head] = interval_us;

    interval_ring_buffer_head =
        (interval_ring_buffer_head + 1) %
        FILTER_WINDOW_SIZE;

    if (interval_ring_buffer_count < FILTER_WINDOW_SIZE) {
        interval_ring_buffer_count++;
    }
}

//--------------------------------------------------
// センサ間隔バッファクリア
//--------------------------------------------------

void clear_sensor_interval_buffer()
{
    interval_ring_buffer_head = 0;
    interval_ring_buffer_count = 0;
}

//--------------------------------------------------
// 平滑化フィルタ
//--------------------------------------------------

uint32_t calculate_smoothed_interval_us()
{
    if (interval_ring_buffer_count == 0) {
        return INVALID_TIME_US;
    }

    uint64_t sum = 0;
    for (int32_t i = 0; i < interval_ring_buffer_count; i++) {
        sum += interval_ring_buffer_us[i];
    }

    return static_cast<uint32_t>(sum / interval_ring_buffer_count);
}


//--------------------------------------------------
// 現在速度取得
//--------------------------------------------------

uint32_t calculate_current_speed_cm_s()
{
    const uint32_t median_interval_us = calculate_smoothed_interval_us();


    const uint32_t current_interval_us = micros() - last_time_us;

    const uint32_t effective_interval_us =
        max(
            median_interval_us,
            current_interval_us
        );

    if (effective_interval_us == 0 ||
        effective_interval_us == INVALID_TIME_US) {
        return 0;
    }

    return static_cast<uint32_t>(static_cast<uint64_t>(LINE_PITCH_MM) * 100000ULL / effective_interval_us);
}

//--------------------------------------------------
// センサ割り込み
//--------------------------------------------------

void IRAM_ATTR sensor_isr(void *arg)
{
    const bool sensor_state =
        digitalRead(SENSOR_PIN);

    // センサがOFF
    if (!sensor_state) {
        is_sensor_enabled = false;
    }
    // センサがON
    else if (!is_sensor_enabled) {

        const uint32_t current_time_us =
            micros();

        const uint32_t interval_us =
            current_time_us - last_time_us;
            line_count++;
            push_sensor_interval_us(interval_us);
            last_time_us = current_time_us;
            is_sensor_enabled = true;
    }
}

//--------------------------------------------------
// センサセットアップ
//--------------------------------------------------

void setup_sensor()
{
    pinMode(SENSOR_PIN, INPUT);

    const gpio_num_t sensor_gpio =
        static_cast<gpio_num_t>(SENSOR_PIN);

    gpio_set_intr_type(
        sensor_gpio,
        GPIO_INTR_ANYEDGE
    );

    gpio_install_isr_service(
        ESP_INTR_FLAG_LEVEL3 |
        ESP_INTR_FLAG_IRAM
    );

    gpio_isr_handler_add(
        sensor_gpio,
        sensor_isr,
        nullptr
    );
}