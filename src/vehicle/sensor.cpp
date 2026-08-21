#include <Arduino.h>

#include "config.hpp"
#include "sensor.hpp"

uint32_t interval_buffer_us[MEDIAN_FILTER_WINDOW_SIZE];

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
        MEDIAN_FILTER_WINDOW_SIZE;

    if (interval_buffer_count < MEDIAN_FILTER_WINDOW_SIZE) {
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

    uint32_t work_buffer_us[MEDIAN_FILTER_WINDOW_SIZE];

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
// 現在速度取得
//--------------------------------------------------

int32_t calculate_current_speed_cm_s()
{
    const uint32_t median_interval_us =
        calculate_median_interval_us();

    const uint32_t current_interval_us =
        micros() - last_time_us;

    const uint32_t effective_interval_us =
        max(
            median_interval_us,
            current_interval_us
        );

    if (effective_interval_us == 0 ||
        effective_interval_us == INVALID_TIME_US) {
        return 0;
    }

    return static_cast<int32_t>(
        static_cast<uint64_t>(LINE_PITCH_MM)
        * 100000ULL
        / effective_interval_us
    );
}

//--------------------------------------------------
// センサ割り込み
//--------------------------------------------------

void IRAM_ATTR sensor_isr()
{
    const bool sensor_state =
        digitalRead(SENSOR_PIN);

    // センサがONになった瞬間
    if (sensor_state && !is_sensor_enabled) {

        const uint32_t current_time_us =
            micros();

        const uint32_t interval_us =
            current_time_us - last_time_us;

        // デバウンス時間以内の入力は無視
        if (interval_us > SENSOR_DEBOUNCE_US) {

            line_count++;

            push_sensor_interval_us(interval_us);

            last_time_us =
                current_time_us;

            sensor_log_index++;

            is_sensor_enabled = true;
        }
    }

    // センサがOFFになった
    else if (!sensor_state) {
        is_sensor_enabled = false;
    }
}

//--------------------------------------------------
// センサセットアップ
//--------------------------------------------------

void setup_sensor()
{
    pinMode(SENSOR_PIN, INPUT);

    attachInterrupt(
        digitalPinToInterrupt(SENSOR_PIN),
        sensor_isr,
        CHANGE
    );
}