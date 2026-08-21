#include <Arduino.h>
#include <LittleFS.h>

#include "config.hpp"
#include "logger.hpp"

namespace {

File log_files[VEHICLE_COUNT];
bool has_log_data[VEHICLE_COUNT] = {false};

constexpr uint32_t LOG_TRANSFER_TIMEOUT_MS = 5000;

} // namespace

//--------------------------------------------------
// フラッシュメモリクリア
//--------------------------------------------------

void clear_flash()
{
    if (!LittleFS.begin()) {
        return;
    }

    File root = LittleFS.open("/");
    File entry = root.openNextFile();

    while (entry) {
        const String path = "/" + String(entry.name());

        entry.close();

        LittleFS.remove(path);

        entry = root.openNextFile();
    }
}

//--------------------------------------------------
// Logger setup
//--------------------------------------------------

void setup_logger()
{
    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS mount failed");
        return;
    }

    clear_flash();
}

//--------------------------------------------------
// ログファイルを開く
//--------------------------------------------------

void open_log_file()
{
    for (int32_t vehicle_index = 0;
         vehicle_index < VEHICLE_COUNT;
         ++vehicle_index) {

        has_log_data[vehicle_index] = false;

        const String filename =
            "/log" + String(vehicle_index) + ".csv";

        log_files[vehicle_index] =
            LittleFS.open(filename, FILE_WRITE);

        if (!log_files[vehicle_index]) {
            Serial.printf(
                "Failed to open %s\n",
                filename.c_str()
            );
            continue;
        }

        Serial.printf(
            "%s opened\n",
            filename.c_str()
        );

        log_files[vehicle_index].println(
            "Time_us,Speed_cm_s,PWM,EnterTime_us,ExitTime_us"
        );
    }
}

//--------------------------------------------------
// ログ書き込み
//--------------------------------------------------

void write_log(
    int32_t vehicle_index,
    const StatusData& data
)
{
    if (vehicle_index < 0 ||
        vehicle_index >= VEHICLE_COUNT) {
        return;
    }

    if (!log_files[vehicle_index]) {
        return;
    }

    has_log_data[vehicle_index] = true;

    log_files[vehicle_index].print(data.timestamp_us);
    log_files[vehicle_index].print(",");

    log_files[vehicle_index].print(data.speed_cm_s);
    log_files[vehicle_index].print(",");

    log_files[vehicle_index].print(data.pwm);
    log_files[vehicle_index].print(",");

    if (data.time_to_enter_intersection_us != INVALID_TIME_US) {
        log_files[vehicle_index].print(data.time_to_enter_intersection_us);
    }

    log_files[vehicle_index].print(",");

    if (data.time_to_exit_intersection_us != INVALID_TIME_US) {
        log_files[vehicle_index].print(data.time_to_exit_intersection_us);
    }

    log_files[vehicle_index].println();
}

//--------------------------------------------------
// ログファイルを閉じる
//--------------------------------------------------

void close_log_file()
{
    for (int32_t vehicle_index = 0;
         vehicle_index < VEHICLE_COUNT;
         ++vehicle_index) {

        if (log_files[vehicle_index]) {
            log_files[vehicle_index].close();
        }
    }

    Serial.println("All logs ready.");
}

//--------------------------------------------------
// ログ転送
//--------------------------------------------------

void send_log()
{
    is_logging = true;

    int32_t log_count = 0;

    for (int32_t vehicle_index = 0;
         vehicle_index < VEHICLE_COUNT;
         ++vehicle_index) {

        if (has_log_data[vehicle_index]) {
            ++log_count;
        }
    }

    Serial.printf(
        "COUNT %ld\n",
        log_count
    );

    for (int32_t vehicle_index = 0;
         vehicle_index < VEHICLE_COUNT;
         ++vehicle_index) {

        if (!has_log_data[vehicle_index]) {
            continue;
        }

        const String filename =
            "/log" + String(vehicle_index) + ".csv";

        File log_file =
            LittleFS.open(filename, FILE_READ);

        if (!log_file) {
            Serial.printf(
                "Cannot open %s\n",
                filename.c_str()
            );
            continue;
        }

        Serial.printf(
            "BEGIN %ld\n",
            vehicle_index
        );

        while (log_file.available()) {
            Serial.write(log_file.read());
        }

        Serial.println();

        Serial.printf(
            "END %ld\n",
            vehicle_index
        );

        log_file.close();
    }

    // PythonからACKを待つ
    const uint32_t start_time_ms = millis();

    while (millis() - start_time_ms <
           LOG_TRANSFER_TIMEOUT_MS) {

        if (!Serial.available()) {
            continue;
        }

        String command =
            Serial.readStringUntil('\n');

        command.trim();

        if (command != "ACK") {
            continue;
        }

        for (int32_t vehicle_index = 0;
             vehicle_index < VEHICLE_COUNT;
             ++vehicle_index) {

            if (!has_log_data[vehicle_index]) {
                continue;
            }

            const String filename =
                "/log" + String(vehicle_index) + ".csv";

            LittleFS.remove(filename);
        }

        Serial.println("ALL LOGS DELETED");
        break;
    }

    is_logging = false;
}