#pragma once
#include <stdint.h>
#include <stddef.h>

typedef enum Color {
    COLOR_BLACK = 0,
    COLOR_WHITE = 15,
} Color;

void pchar(const char c);

void clear_screen();

void kprint(const char* str);

void kprint_num_u8(uint8_t num);

void kprint_num_u32(uint32_t num);

void set_color(const uint8_t foreground, const uint8_t background);