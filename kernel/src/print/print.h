#pragma once
#include <stdint.h>
#include <stddef.h>

void kprint_init(void);

void pchar(char c, uint16_t x, uint16_t y);

void kprint(char* str);