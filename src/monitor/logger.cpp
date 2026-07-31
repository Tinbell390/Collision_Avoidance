#include <Arduino.h>
#include <LittleFS.h>
#include "config.hpp"
#include "logger.hpp"

//モニタ用
File file[vehiclecount];
bool fileHasData[vehiclecount];   // 実際にSTATUSを受信して記録したかどうか

//--------------------------------------------------
//フラッシュメモリクリア関数
//--------------------------------------------------
void clearFlash(){
    if (!LittleFS.begin()){
        return ;
    }

    File root = LittleFS.open("/");
    File file = root.openNextFile();

    while (file){
        String path = "/" + String(file.name());
        file.close();

        LittleFS.remove(path);

        file = root.openNextFile();
    }
}

//--------------------------------------------------
// セットアップ関数
//--------------------------------------------------
void setup_logger(){
    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS mount failed");
    }
    clearFlash();
}



//--------------------------------------------------
// ファイル展開関数 (START)
//--------------------------------------------------
void OpenFile(){
    for (int i = 0; i < vehiclecount; i++) {

        fileHasData[i] = false;   // 新しい試行のためリセット

        String filename = "/log" + String(i) + ".csv";

        file[i] = LittleFS.open(filename, FILE_WRITE);

        if (!file[i]) {
            Serial.printf("Failed to open %s\n", filename.c_str());
            continue;
        }

        Serial.printf("%s opened\n", filename.c_str());

        // ヘッダ
        file[i].println("Time_us,Speed,PWM");
    }
}

//--------------------------------------------------
// ログ書き込み関数 (STATUS受信)
//--------------------------------------------------
void WriteLog(int i, StatusData data){

    if (!file[i]) {
        return;
    }

    fileHasData[i] = true;   // このファイルは実データを持つ

    file[i].print(data.time_us);
    file[i].print(",");
    file[i].print(data.speed);
    file[i].print(",");
    file[i].println(data.pwm);
}

//--------------------------------------------------
// ファイル閉じ関数 (FINISH)
//--------------------------------------------------
void CloseFile(){

    for(int i = 0; i < vehiclecount; i++){
        if(file[i]){
            file[i].close();
        }
    }

    Serial.println("All logs ready.");
}

//--------------------------------------------------
// ログ転送関数
//--------------------------------------------------
void sendLog(){

    // 実際にデータがある車両数を数える
    int sendCount = 0;
    for(int i = 0; i < vehiclecount; i++){
        if (fileHasData[i]) sendCount++;
    }

    Serial.printf("COUNT %d\n", sendCount);

    for(int i = 0; i < vehiclecount; i++){

        if (!fileHasData[i]) continue;   // 記録がない車両はスキップ

        String filename = "/log" + String(i) + ".csv";

        File f = LittleFS.open(filename, FILE_READ);

        if(!f){
            Serial.printf("Cannot open %s\n", filename.c_str());
            continue;
        }

        Serial.printf("BEGIN %d\n", i);

        while(f.available()){
            Serial.write(f.read());
        }

        Serial.println();
        Serial.printf("END %d\n", i);

        f.close();
    }

    // PythonからACK待ち
    uint32_t start = millis();

    while(millis() - start < 5000){

        if(Serial.available()){

            String cmd = Serial.readStringUntil('\n');
            cmd.trim();

            if(cmd == "ACK"){

                for(int i = 0; i < vehiclecount; i++){
                    if (!fileHasData[i]) continue;   // 記録がある分だけ削除

                    String filename = "/log" + String(i) + ".csv";
                    LittleFS.remove(filename);
                }

                Serial.println("ALL LOGS DELETED");
                break;
            }
        }
    }
}