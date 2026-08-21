#ifndef LOGGER_HPP
#define LOGGER_HPP
#include"config.hpp"

void setup_logger();
void open_log_file();
void write_log(int32_t vehicle_index,const StatusData& data);
void close_log_file();
void send_log();

#endif