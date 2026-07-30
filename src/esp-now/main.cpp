#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

// 実験1で使用する
// esp-nowの導通確認実験

void AddPeer(const uint8_t *mac_addr);
void PrintMacAddress(const uint8_t *mac_addr);
void SendHello();
void SendAck(const uint8_t *mac_addr);
void SendPing(const uint8_t *mac_addr);
void SendPong(const uint8_t *mac_addr);
void SendDataRequest(const uint8_t *mac_addr);
void SendData(const uint8_t *mac_addr);
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int len);

uint32_t PINGSTART;
uint32_t DATASTART;

// パケット種別（全パケット共通のヘッダとして使う）
enum PacketType : uint8_t{
    HELLO,
    ACK,
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

// ヘッダのみのパケット（HELLO, ACK, PING, PONG, DATA_REQ用）
struct Packet{
    PacketHeader header;
};

// ダミーデータのサイズ（実際の通信量を模擬するためのサイズ）
#define DUMMY_DATA_SIZE 32

// データパケット（ヘッダ + ダミーデータ本体）
struct DataPacket{
    PacketHeader header;
    uint8_t data[DUMMY_DATA_SIZE];
};

uint8_t broadcastAddress[6] = {
    0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF
};

uint8_t unicastAddress[6];

void setup(){
    // シリアル通信開始
    Serial.begin(115200);

    // Wi-FiをSTAモードに設定
    WiFi.mode(WIFI_STA);

    // ESP-NOW初期化
    if (esp_now_init() != ESP_OK)    {
        Serial.println("ESP-NOW Init Failed");
        while (true);
    }

    // コールバック関数登録
    esp_now_register_recv_cb(OnDataRecv);

    // ブロードキャストアドレスをPeer登録
    AddPeer(broadcastAddress);

    Serial.println("ESP-NOW Ready");
}

//--------------------------------------------------
// Peer登録
//--------------------------------------------------
void AddPeer(const uint8_t *mac_addr){
    Serial.print("Add Peer : ");
    PrintMacAddress(mac_addr);

    if(esp_now_is_peer_exist(mac_addr))    {
        Serial.println("Peer already exists");
        return;
    }


    esp_now_peer_info_t peerInfo = {};

    memcpy(peerInfo.peer_addr, mac_addr, 6);

    peerInfo.channel = 0;
    peerInfo.encrypt = false;


    esp_err_t result = esp_now_add_peer(&peerInfo);


    if(result == ESP_OK)    {
        Serial.println("Peer Added");
    }
    else    {
        Serial.print("Peer Add Failed : ");
        Serial.println(result);
    }
}

//--------------------------------------------------
// HELLO送信（ブロードキャスト）
//--------------------------------------------------
void SendHello(){
    Packet packet;
    packet.header.type = HELLO;

    Serial.println();
    if (esp_now_send(broadcastAddress, (uint8_t *)&packet, sizeof(packet)) == ESP_OK){
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
void PrintMacAddress(const uint8_t *mac_addr){
    for (int i = 0; i < 6; i++){
        Serial.printf("%02X", mac_addr[i]);
        if (i != 5) Serial.print(":");
    }
    Serial.println();
}


//--------------------------------------------------
// ACK送信（ユニキャスト）
//--------------------------------------------------
void SendAck(const uint8_t *mac_addr){
    Packet packet;
    packet.header.type = ACK;

    Serial.print("Send : ACK -> ");
    PrintMacAddress(mac_addr);

    if (esp_now_send(mac_addr, (uint8_t *)&packet, sizeof(packet)) == ESP_OK){
        Serial.println("ACK Send OK");
    }
    else{
        Serial.println("ACK Send Failed");
    }
}


//--------------------------------------------------
// PING送信（ユニキャスト）
//--------------------------------------------------
void SendPing(const uint8_t *mac_addr){
    Packet packet;
    packet.header.type = PING;
    PINGSTART = millis();

    Serial.println();
    Serial.print("Send : PING -> ");
    PrintMacAddress(mac_addr);

    if (esp_now_send(mac_addr, (uint8_t *)&packet, sizeof(packet)) == ESP_OK){
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
void SendPong(const uint8_t *mac_addr){
    Packet packet;
    packet.header.type = PONG;

    Serial.print("Send : PONG -> ");
    PrintMacAddress(mac_addr);

    if (esp_now_send(mac_addr, (uint8_t *)&packet, sizeof(packet)) == ESP_OK){
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
void SendDataRequest(const uint8_t *mac_addr){
    Packet packet;
    packet.header.type = DATA_REQ;
    DATASTART = millis();

    Serial.println();
    Serial.print("Send : DATA_REQ -> ");
    PrintMacAddress(mac_addr);

    if (esp_now_send(mac_addr, (uint8_t *)&packet, sizeof(packet)) == ESP_OK){
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
void SendData(const uint8_t *mac_addr){
    DataPacket packet;
    packet.header.type = DATA;

    // ダミーデータ生成（中身に意味はない）
    for (int i = 0; i < DUMMY_DATA_SIZE; i++){
        packet.data[i] = (uint8_t)i;
    }

    Serial.print("Send : DATA -> ");
    PrintMacAddress(mac_addr);

    if (esp_now_send(mac_addr, (uint8_t *)&packet, sizeof(packet)) == ESP_OK){
        Serial.println("DATA Send OK");
    }
    else{
        Serial.println("DATA Send Failed");
    }
}

//--------------------------------------------------
// 受信コールバック
//--------------------------------------------------
void OnDataRecv(const uint8_t *mac_addr,const uint8_t *data,int len){
    // ヘッダ長すら無い場合は不正パケットとして破棄
    if (len < (int)sizeof(PacketHeader)){
        Serial.println("Invalid Packet Size");
        return;
    }

    // まずヘッダだけを見て種別を判定する
    const PacketHeader *header = (const PacketHeader *)data;

    switch(header->type){
        case HELLO:{
            if (len != sizeof(Packet)){
                Serial.println("Invalid HELLO Packet Size");
                break;
            }

            Serial.println();
            Serial.print("Recv : HELLO from ");
            
            PrintMacAddress(mac_addr);

            // 相手をPeer登録
            AddPeer(mac_addr);

            // ACK返信
            SendAck(mac_addr);

            Serial.println();
            break;
        }


        case ACK:{
            if (len != sizeof(Packet)){
                Serial.println("Invalid ACK Packet Size");
                break;
            }

            Serial.println();
            Serial.print("Recv : ACK from ");
            PrintMacAddress(mac_addr);
            Serial.println();
            break;
        }


        case PING:{
            if (len != sizeof(Packet)){
                Serial.println("Invalid PING Packet Size");
                break;
            }

            Serial.println();
            Serial.print("Recv : PING from ");
            PrintMacAddress(mac_addr);

            // PONG返信
            SendPong(mac_addr);
            Serial.println();
            break;
        }


        case PONG:{
            if (len != sizeof(Packet)){
                Serial.println("Invalid PONG Packet Size");
                break;
            }

            uint32_t elapsed = millis() - PINGSTART;

            Serial.println();
            Serial.print("Recv : PONG from ");
            PrintMacAddress(mac_addr);

            Serial.print("RTT : ");
            Serial.print(elapsed);
            Serial.println(" ms");
            Serial.println();
            break;
        }


        case DATA_REQ:{
            if (len != sizeof(Packet)){
                Serial.println("Invalid DATA_REQ Packet Size");
                break;
            }

            Serial.println();
            Serial.print("Recv : DATA_REQ from ");
            PrintMacAddress(mac_addr);

            // 要求されたのでデータ本体を返信
            SendData(mac_addr);

            Serial.println();
            break;
        }


        case DATA:{
            if (len != sizeof(DataPacket)){
                Serial.println("Invalid DATA Packet Size");
                break;
            }

            uint32_t elapsed = millis() - DATASTART;

            Serial.println();
            Serial.print("Recv : DATA from ");
            PrintMacAddress(mac_addr);

            Serial.print("Data Size : ");
            Serial.print(DUMMY_DATA_SIZE);
            Serial.println(" bytes");

            Serial.print("RTT : ");
            Serial.print(elapsed);
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
    if (Serial.available()){
        String command = Serial.readStringUntil('\n');
        command.trim();

        // HELLO送信
        if (command == "HELLO"){
            SendHello();
        }

        // Peer登録
        else if (command.startsWith("addpeer")){
            uint8_t mac[6];

            // "addpeer "以降を取得
            String macString = command.substring(8);

            if (sscanf(macString.c_str(),
                       "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                       &mac[0], &mac[1], &mac[2],
                       &mac[3], &mac[4], &mac[5]) == 6){
                Serial.print("Add Peer : ");
                PrintMacAddress(mac);

                AddPeer(mac);
            }
            else{
                Serial.println("Invalid MAC Address");
            }
        }

        // PING送信
        else if (command.startsWith("PING")){
            uint8_t mac[6];

            // "PING "以降を取得
            String macString = command.substring(5);

            if (sscanf(macString.c_str(),
                       "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                       &mac[0], &mac[1], &mac[2],
                       &mac[3], &mac[4], &mac[5]) == 6){
                SendPing(mac);
            }
            else{
                Serial.println("Invalid MAC Address");
            }
        }

        // DATAREQ送信
        else if (command.startsWith("DATAREQ")){
            uint8_t mac[6];

            // "DATAREQ "以降を取得
            String macString = command.substring(8);

            if (sscanf(macString.c_str(),
                       "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                       &mac[0], &mac[1], &mac[2],
                       &mac[3], &mac[4], &mac[5]) == 6){
                SendDataRequest(mac);
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