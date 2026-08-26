#include "config.hpp"
#include "prediction.hpp"
#include "collision_avoidance.hpp"

constexpr uint64_t MICROSECONDS_PER_SECOND = 1000000ULL;
constexpr int32_t MM_PER_CM = 10;

//--------------------------------------------------
// 衝突回避 / 意図的衝突の目標速度算出
//
// other_enter_time_us:
//     相手が交差点へ突入するまでの残り時間 [us]
//
// other_exit_time_us:
//     相手が交差点から離脱するまでの残り時間 [us]
// 
// other_timestamp_us:
//     相手のタイムスタンプ [us]
// 
// is_collision_detected == true:
//     相手と同時に交差点へ突入する
//
// is_collision_detected == false:
//     相手と交差点内で干渉しないようにする
//--------------------------------------------------
int32_t calculate_collision_avoidance_speed_cm_s(uint32_t current_time_us){

    // 相手の情報が無効(未受信 or 使用済み)なら回避ロジックを走らせない
    if (other_enter_time_us == INVALID_TIME_US || other_exit_time_us  == INVALID_TIME_US || other_timestamp_us  == INVALID_TIME_US) {
        return target_speed_cm_s;   
    }

    // 相手が交差点から離脱していれば初期速度に戻す
    if(other_exit_time_us == 0){
        return DEFAULT_SPEED_CM_S;
    }

    // 相手の交差点突入までの残り時間 [us]をタイムスタンプで補正する
    uint32_t other_enter_remaining_us = other_enter_time_us + other_timestamp_us - (current_time_us);   // 相手の交差点突入までの残り時間 [us]
    uint32_t other_exit_remaining_us = other_exit_time_us + other_timestamp_us - (current_time_us);     // 相手の交差点離脱までの残り時間 [us]

    //--------------------------------------------------
    // 現在位置 [mm]
    //--------------------------------------------------
    const int32_t current_position_mm = line_count * LINE_PITCH_MM;

    //--------------------------------------------------
    // 交差点入口までの残り距離 [mm]
    //--------------------------------------------------
    const int32_t remaining_distance_mm = DIST_TO_INTERSECTION_ENTRY_MM - current_position_mm;

    //--------------------------------------------------
    // すでに交差点入口を通過している場合
    //--------------------------------------------------
    if (remaining_distance_mm <= 0) {
        return target_speed_cm_s;
    }

    //--------------------------------------------------
    // 現在速度が0の場合
    //--------------------------------------------------
    if (current_speed_cm_s <= 0) {
        return target_speed_cm_s;
    }

    //--------------------------------------------------
    // 自車の現在速度での交差点突入までの残り時間
    //
    // prediction.cppから取得
    //--------------------------------------------------
    const uint32_t my_enter_remaining_us = self_enter_time_us;

    //--------------------------------------------------
    // 自車の現在速度での交差点離脱までの残り時間
    //--------------------------------------------------
    const uint32_t my_exit_remaining_us =self_exit_time_us;

    // -------------------------------------------------
    // 自車の情報が無効なら現在速度を維持する
    //--------------------------------------------------
    if(my_enter_remaining_us == INVALID_TIME_US || my_exit_remaining_us  == INVALID_TIME_US || my_enter_remaining_us>my_exit_remaining_us){ 
        return target_speed_cm_s;
    }

    //--------------------------------------------------
    // 自車が交差点内に滞在する時間 [us]
    //--------------------------------------------------
    uint32_t my_intersection_time_us = my_exit_remaining_us - my_enter_remaining_us;


    //--------------------------------------------------
    // 目標突入までの残り時間 [us]
    //--------------------------------------------------
    int64_t desired_enter_remaining_us = 0;

    // -------------------------------------------------
    // 自車と相手の到着までの時間差 [us]
    //--------------------------------------------------
    int64_t separation_time_us = (my_enter_remaining_us <= other_enter_remaining_us)?
        static_cast<int64_t>(other_enter_remaining_us) - static_cast<int64_t>(my_exit_remaining_us): 
        static_cast<int64_t>(my_enter_remaining_us) - static_cast<int64_t>(other_exit_remaining_us);

    //--------------------------------------------------
    // CASE 1
    //
    // 意図的に衝突させる
    //
    // 相手の突入までの残り時間と
    // 自車の突入までの残り時間を一致させる。
    //--------------------------------------------------
    if (is_collision_detected) {
        desired_enter_remaining_us = static_cast<int64_t>(other_enter_remaining_us);
    }

    // -------------------------------------------------
    // 相手と自分の到着までの時間がマージン以上なら現在の目標速度を返す
    // --------------------------------------------------
    else if (separation_time_us >= static_cast<int64_t>(TIME_MARGIN_US)) {
        return target_speed_cm_s;
    }

    //--------------------------------------------------
    // CASE 2
    //
    // 衝突回避
    //
    // 自車が現在速度のままだと相手より先に
    // 交差点へ突入する場合。
    //
    // 自車が交差点を通過し終わってから
    // 相手が突入するようにする。
    //--------------------------------------------------
    else if (static_cast<uint64_t>(my_enter_remaining_us) < static_cast<uint64_t>(other_enter_remaining_us)) {
        desired_enter_remaining_us =
            static_cast<int64_t>(other_enter_remaining_us)
            - static_cast<int64_t>(my_intersection_time_us)
            - static_cast<int64_t>(TIME_MARGIN_US);
    }

    //--------------------------------------------------
    // CASE 3
    //
    // 衝突回避
    //
    // 相手が先に突入する場合。
    //
    // 相手が離脱してから
    // TIME_MARGIN_US 後に自車が突入する。
    //--------------------------------------------------
    else {
        desired_enter_remaining_us =
            static_cast<int64_t>(other_exit_remaining_us)
            + static_cast<int64_t>(TIME_MARGIN_US);
    }

    //--------------------------------------------------
    // 目標時間が0以下の場合
    //
    // これ以上減速しても目標時間には間に合わない。
    //--------------------------------------------------
    if (desired_enter_remaining_us <= 0) {
        return target_speed_cm_s;
    }

    //--------------------------------------------------
    // 必要速度を計算
    //
    // distance [mm]
    // ----------------
    // time [s]
    //
    // = mm/s
    //
    // さらに / 10 して cm/s に変換する。
    //--------------------------------------------------
    const uint64_t required_speed_cm_s =
        static_cast<uint64_t>(remaining_distance_mm) * MICROSECONDS_PER_SECOND 
        / static_cast<uint64_t>(desired_enter_remaining_us) / MM_PER_CM;

    //--------------------------------------------------
    // MIN～MAXに制限
    //--------------------------------------------------
    const int32_t target_speed_cm_s = constrain(static_cast<int32_t>(required_speed_cm_s),MIN_SPEED_CM_S,MAX_SPEED_CM_S);

    //--------------------------------------------------
    // 処理後に受信した他車の情報を破棄する
    //--------------------------------------------------
    other_enter_time_us = INVALID_TIME_US;
    other_exit_time_us = INVALID_TIME_US;
    other_timestamp_us = INVALID_TIME_US;

    return target_speed_cm_s;
}