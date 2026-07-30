#ifndef LOGGER_HPP
#define LOGGER_HPP

#include"config.hpp"

void setup_logger();
void OpenFile();
void WriteLog(int i, StatusData data);
void CloseFile();
void sendLog();
#endif