#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "config.hpp"

// 実験1で使用する
// esp-nowの導通確認実験

void add_peer(const uint8_t *mac_addr);
void print_mac_address(const uint8_t *mac_addr);
void send_hello();
void send_ping(const uint8_t *mac_addr);
void send_pong(const uint8_t *mac_addr);
void send_data_request(const uint8_t *mac_addr);
void send_data(const uint8_t *mac_addr);
void on_data_recv(const uint8_t *mac_addr, const uint8_t *data, int len);

// ------------------------------
// コマンド文字列の固定長プレフィックス
// ------------------------------
constexpr size_t ADDPEER_PREFIX_LEN = 8;  // "addpeer "
constexpr size_t PING_PREFIX_LEN    = 5;  // "PING "
constexpr size_t DATAREQ_PREFIX_LEN = 8;  // "DATAREQ "

// ------------------------------
// ダミーデータ生成用パラメータ
// ------------------------------
constexpr int32_t DUMMY_SPEED_MIN_CM_S   = 500;
constexpr int32_t DUMMY_SPEED_RANGE_CM_S = 500;
constexpr uint8_t DUMMY_PWM_MIN          = 50;
constexpr uint8_t DUMMY_PWM_RANGE        = 100;
constexpr uint32_t DUMMY_ENTER_OFFSET_MS = 1000;
constexpr uint32_t DUMMY_EXIT_OFFSET_MS  = 500;

// PING/DATA_REQを送信した時刻（RTT計算用）
uint32_t ping_start_time_ms;
uint32_t data_start_time_ms;

// パケット種別（全パケット共通のヘッダとして使う）
enum class PacketType : uint8_t{
    HELLO,
    PING,
    PONG,
    DATA_REQ,   // データ要求
    DATA        // データ本体（ダミーデータ）
};

// パケットヘッダ
// すべてのパケットは先頭にこのヘッダを持つ
struct PacketHeader{
    PacketType type;
};

// ヘッダのみのパケット（HELLO, PING, PONG, DATA_REQ用）
struct Packet{
    PacketHeader header;
};

// データパケット（ヘッダ + ダミーデータ本体）
struct DataPacket{
    PacketHeader header;
    StatusData payload;
};

uint8_t broadcast_address[6] = {
    0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF
};

uint8_t unicast_address[6];

void setup(){
    // シリアル通信開始
    Serial.begin(115200);

    // Wi-FiをSTAモードに設定
    WiFi.mode(WIFI_STA);

    // ESP-NOW初期化
    if (esp_now_init() != ESP_OK){
        Serial.println("ESP-NOW Init Failed");
        while (true);
    }

    // コールバック関数登録
    esp_now_register_recv_cb(on_data_recv);

    // ブロードキャストアドレスをPeer登録
    add_peer(broadcast_address);

    Serial.println("ESP-NOW Ready");
}

//--------------------------------------------------
// Peer登録
//--------------------------------------------------
void add_peer(const uint8_t *mac_addr){
    Serial.print("Add Peer : ");
    print_mac_address(mac_addr);

    if(esp_now_is_peer_exist(mac_addr)){
        Serial.println("Peer already exists");
        return;
    }

    esp_now_peer_info_t peer_info = {};

    ①;

    peer_info.channel = 0;
    peer_info.encrypt = false;

    const esp_err_t result = esp_now_add_peer(&peer_info);

    if(result == ESP_OK){
        Serial.println("Peer Added");
    }
    else{
        Serial.print("Peer Add Failed : ");
        Serial.println(result);
    }
}

//--------------------------------------------------
// HELLO送信（ブロードキャスト）
//--------------------------------------------------
void send_hello(){
    Packet packet;
    packet.header.type = PacketType::HELLO;

    Serial.println();
    if(esp_now_send(broadcast_address, ②, sizeof(packet)) == ESP_OK){
        Serial.println("Send : HELLO");
    }
    else{
        Serial.println("HELLO Send Failed");
    }
    Serial.println();
}

//--------------------------------------------------
// MACアドレス表示
//--------------------------------------------------
void print_mac_address(const uint8_t *mac_addr){
    for (int i = 0; i < 6; i++){
        Serial.printf("%02X", mac_addr[i]);
        if (i != 5) Serial.print(":");
    }
    Serial.println();
}

//--------------------------------------------------
// PING送信（ユニキャスト）
//--------------------------------------------------
void send_ping(const uint8_t *mac_addr){
    Packet packet;
    packet.header.type = PacketType::PING;
    ping_start_time_ms = millis();

    Serial.println();
    Serial.print("Send : PING -> ");
    print_mac_address(mac_addr);

    if(esp_now_send(mac_addr, ②, sizeof(packet)) == ESP_OK){
        Serial.println("PING Send OK");
    }
    else{
        Serial.println("PING Send Failed");
    }
    Serial.println();
}


//--------------------------------------------------
// PONG送信（ユニキャスト）
//--------------------------------------------------
void send_pong(const uint8_t *mac_addr){
    Packet packet;
    packet.header.type = PacketType::PONG;

    Serial.print("Send : PONG -> ");
    print_mac_address(mac_addr);

    if(esp_now_send(mac_addr, ②, sizeof(packet)) == ESP_OK){
        Serial.println("PONG Send OK");
    }
    else{
        Serial.println("PONG Send Failed");
    }
}


//--------------------------------------------------
// DATA_REQ送信（ユニキャスト）
// 相手に対してデータ本体の送信を要求する
//--------------------------------------------------
void send_data_request(const uint8_t *mac_addr){
    Packet packet;
    packet.header.type = PacketType::DATA_REQ;
    data_start_time_ms = millis();

    Serial.println();
    Serial.print("Send : DATA_REQ -> ");
    print_mac_address(mac_addr);

    if(esp_now_send(mac_addr, ②, sizeof(packet)) == ESP_OK){
        Serial.println("DATA_REQ Send OK");
    }
    else{
        Serial.println("DATA_REQ Send Failed");
    }
    Serial.println();
}


//--------------------------------------------------
// DATA送信（ユニキャスト）
// ダミーデータ本体を送信する
//--------------------------------------------------
void send_data(const uint8_t *mac_addr){

    DataPacket packet;

    packet.header.type = PacketType::DATA;


    // -----------------------------
    // ダミーデータ生成
    // -----------------------------

    static uint32_t send_count = 0;

    packet.payload.timestamp_us = micros();

    // 速度 500〜1000 の範囲で変化
    packet.payload.speed_cm_s = DUMMY_SPEED_MIN_CM_S + (send_count % DUMMY_SPEED_RANGE_CM_S);

    // PWM 50〜150
    packet.payload.pwm = DUMMY_PWM_MIN + (send_count % DUMMY_PWM_RANGE);

    // 交差点突入時間
    packet.payload.time_to_enter_intersection_us = millis() + DUMMY_ENTER_OFFSET_MS;

    // 交差点退出時間
    packet.payload.time_to_exit_intersection_us = packet.payload.time_to_enter_intersection_us + DUMMY_EXIT_OFFSET_MS;

    send_count++;


    Serial.println();
    Serial.print("Send : DATA -> ");
    print_mac_address(mac_addr);

    Serial.println("Dummy Data:");
    Serial.print(" time_us   : ");
    Serial.println(packet.payload.timestamp_us);

    Serial.print(" speed     : ");
    Serial.println(packet.payload.speed_cm_s);

    Serial.print(" pwm       : ");
    Serial.println(packet.payload.pwm);

    Serial.print(" enterTime : ");
    Serial.println(packet.payload.time_to_enter_intersection_us);

    Serial.print(" exitTime  : ");
    Serial.println(packet.payload.time_to_exit_intersection_us);

    if (esp_now_send(mac_addr,②,sizeof(packet)) == ESP_OK){
        Serial.println("DATA Send OK");
    }
    else{
        Serial.println("DATA Send Failed");
    }
}

//--------------------------------------------------
// 受信コールバック
//--------------------------------------------------
void on_data_recv(const uint8_t *mac_addr,const uint8_t *data,int len){
    // ヘッダ長すら無い場合は不正パケットとして破棄
    if(len < (int)sizeof(PacketHeader)){
        Serial.println("Invalid Packet Size");
        return;
    }

    // まずヘッダだけを見て種別を判定する
    const PacketHeader *header = (const PacketHeader *)data;

    switch(③){
        case PacketType::HELLO:{
            if (len != sizeof(Packet)){
                Serial.println("Invalid HELLO Packet Size");
                break;
            }

            Serial.println();
            Serial.print("Recv : HELLO from ");

            print_mac_address(mac_addr);

            Serial.println();
            break;
        }


        case PacketType::PING:{
            if (len != sizeof(Packet)){
                Serial.println("Invalid PING Packet Size");
                break;
            }

            Serial.println();
            Serial.print("Recv : PING from ");
            print_mac_address(mac_addr);

            // PONG返信
            send_pong(mac_addr);
            Serial.println();
            break;
        }


        case PacketType::PONG:{
            if (len != sizeof(Packet)){
                Serial.println("Invalid PONG Packet Size");
                break;
            }

            const uint32_t elapsed_ms = millis() - ping_start_time_ms;

            Serial.println();
            Serial.print("Recv : PONG from ");
            print_mac_address(mac_addr);

            Serial.print("RTT : ");
            Serial.print(elapsed_ms);
            Serial.println(" ms");
            Serial.println();
            break;
        }


        case PacketType::DATA_REQ:{
            if (len != sizeof(Packet)){
                Serial.println("Invalid DATA_REQ Packet Size");
                break;
            }

            Serial.println();
            Serial.print("Recv : DATA_REQ from ");
            print_mac_address(mac_addr);

            // 要求されたのでデータ本体を返信
            send_data(mac_addr);

            Serial.println();
            break;
        }


        case PacketType::DATA:{
            if (len != sizeof(DataPacket)){
                Serial.println("Invalid DATA Packet Size");
                break;
            }

            const uint32_t elapsed_ms = millis() - data_start_time_ms;

            Serial.println();
            Serial.print("Recv : DATA from ");
            print_mac_address(mac_addr);

            DataPacket *recv_packet = ④;

            Serial.println("DATA:");

            Serial.print(" time_us   : ");
            Serial.println(recv_packet->payload.timestamp_us);

            Serial.print(" speed     : ");
            Serial.println(recv_packet->payload.speed_cm_s);

            Serial.print(" pwm       : ");
            Serial.println(recv_packet->payload.pwm);

            Serial.print(" enterTime : ");
            Serial.println(recv_packet->payload.time_to_enter_intersection_us);

            Serial.print(" exitTime  : ");
            Serial.println(recv_packet->payload.time_to_exit_intersection_us);

            Serial.print("RTT : ");
            Serial.print(elapsed_ms);
            Serial.println(" ms");
            Serial.println();
            break;
        }


        default:{
            Serial.println();
            Serial.println("Unknown Packet");
            Serial.println();
            break;
        }
    }
}

//--------------------------------------------------
// メインループ
//--------------------------------------------------
void loop(){
    if(Serial.available()){
        String command = Serial.readStringUntil('\n');
        command.trim();

        // HELLO送信
        if(command == "HELLO"){
            send_hello();
        }

        // Peer登録
        else if(command.startsWith("addpeer")){
            uint8_t mac_address[6];

            // "addpeer "以降を取得
            const String mac_string = command.substring(ADDPEER_PREFIX_LEN);

            if(sscanf(mac_string.c_str(),
                       "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                       &mac_address[0], &mac_address[1], &mac_address[2],
                       &mac_address[3], &mac_address[4], &mac_address[5]) == 6){
                Serial.print("Add Peer : ");
                print_mac_address(mac_address);

                add_peer(mac_address);
            }
            else{
                Serial.println("Invalid MAC Address");
            }
        }

        // PING送信
        else if(command.startsWith("PING")){
            uint8_t mac_address[6];

            // "PING "以降を取得
            const String mac_string = command.substring(PING_PREFIX_LEN);

            if(sscanf(mac_string.c_str(),
                       "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                       &mac_address[0], &mac_address[1], &mac_address[2],
                       &mac_address[3], &mac_address[4], &mac_address[5]) == 6){
                send_ping(mac_address);
            }
            else{
                Serial.println("Invalid MAC Address");
            }
        }

        // DATAREQ送信
        else if(command.startsWith("DATAREQ")){
            uint8_t mac_address[6];

            // "DATAREQ "以降を取得
            const String mac_string = command.substring(DATAREQ_PREFIX_LEN);

            if(sscanf(mac_string.c_str(),
                       "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                       &mac_address[0], &mac_address[1], &mac_address[2],
                       &mac_address[3], &mac_address[4], &mac_address[5]) == 6){
                send_data_request(mac_address);
            }
            else{
                Serial.println("Invalid MAC Address");
            }
        }

        else{
            Serial.println("Unknown Command");
        }
    }
}

//穴埋め問題
// ①
// 1. memcpy(peer_info.peer_addr, mac_addr, 6)
// 2. peer_info.peer_addr = mac_addr
// 3. strcpy(peer_info.peer_addr, mac_addr)
// 4. *peer_info.peer_addr = *mac_addr
//
// ②
// 1. (uint8_t *)&packet
// 2. packet
// 3. &packet
// 4. (uint8_t *)packet
//
// ③
// 1. header.type
// 2. data.type
// 3. header->type
// 4. len
//
// ④
// 1. (DataPacket *)data
// 2. (Packet *)data
// 3. data
// 4. (StatusData *)data