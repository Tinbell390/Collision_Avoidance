#include <Arduino.h>
#include "driver/gpio.h"
#include "esp_intr_alloc.h"
#include "config.hpp"
#include "sensor.hpp"
const uint8_t FILTER_WINDOW_SIZE = SMOOTH_FILTER_ENABLED ? SMOOTH_FILTER_WINDOW_SIZE : MEDIAN_FILTER_WINDOW_SIZE;
uint32_t interval_buffer_us[FILTER_WINDOW_SIZE];

bool is_sensor_enabled = false;

int32_t interval_buffer_head = 0;
int32_t interval_buffer_count = 0;



//--------------------------------------------------
// センサ間隔データ追加
//--------------------------------------------------

void push_sensor_interval_us(uint32_t interval_us)
{
    interval_buffer_us[interval_buffer_head] = interval_us;

    interval_buffer_head =
        (interval_buffer_head + 1) %
        FILTER_WINDOW_SIZE;

    if (interval_buffer_count < FILTER_WINDOW_SIZE) {
        interval_buffer_count++;
    }
}

//--------------------------------------------------
// センサ間隔バッファクリア
//--------------------------------------------------

void clear_sensor_interval_buffer()
{
    interval_buffer_head = 0;
    interval_buffer_count = 0;
}

//--------------------------------------------------
// メディアンフィルタ
//--------------------------------------------------

uint32_t calculate_median_interval_us()
{
    if (interval_buffer_count == 0) {
        return INVALID_TIME_US;
    }

    uint32_t work_buffer_us[FILTER_WINDOW_SIZE];

    for (int32_t i = 0; i < interval_buffer_count; i++) {
        work_buffer_us[i] =
            interval_buffer_us[i];
    }

    for (int32_t i = 0;
         i < interval_buffer_count - 1;
         i++) {

        for (int32_t j = i + 1;
             j < interval_buffer_count;
             j++) {

            if (work_buffer_us[i] >
                work_buffer_us[j]) {

                const uint32_t temporary_us =
                    work_buffer_us[i];

                work_buffer_us[i] =
                    work_buffer_us[j];

                work_buffer_us[j] =
                    temporary_us;
            }
        }
    }

    return work_buffer_us[
        interval_buffer_count / 2
    ];
}

//--------------------------------------------------
// 平滑化フィルタ
//--------------------------------------------------

uint32_t calculate_smoothed_interval_us()
{
    if (interval_buffer_count == 0) {
        return INVALID_TIME_US;
    }

    uint64_t sum = 0;
    for (int32_t i = 0; i < interval_buffer_count; i++) {
        sum += interval_buffer_us[i];
    }

    return static_cast<uint32_t>(sum / interval_buffer_count);
}


uint32_t trim_mean(){
{
    if (interval_buffer_count == 0) {
        return INVALID_TIME_US;
    }

    uint32_t work_buffer_us[FILTER_WINDOW_SIZE];

    for (int32_t i = 0; i < interval_buffer_count; i++) {
        work_buffer_us[i] = interval_buffer_us[i];
    }

    // Sort
    for (int32_t i = 0;
         i < interval_buffer_count - 1;
         i++) {

        for (int32_t j = i + 1;
             j < interval_buffer_count;
             j++) {

            if (work_buffer_us[i] >
                work_buffer_us[j]) {

                const uint32_t temporary_us =
                    work_buffer_us[i];

                work_buffer_us[i] =
                    work_buffer_us[j];

                work_buffer_us[j] =
                    temporary_us;
            }
        }
    }

    // Average the middle 50%
    const int32_t quarter =
        interval_buffer_count / 4;

    const int32_t start =
        quarter;

    const int32_t end =
        interval_buffer_count - quarter;

    uint64_t sum_us = 0;

    for (int32_t i = start; i < end; i++) {
        sum_us += work_buffer_us[i];
    }

    const int32_t count = end - start;

    return (uint32_t)(sum_us / count);
}
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