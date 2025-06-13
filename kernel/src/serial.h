#pragma once

#define COM1 0x3F8          // COM1

uint8_t serial_init();


void serial_log_num_unsigned(uint64_t num);

void serial_log_num_signed(int64_t num);

void serial_log(char* str);