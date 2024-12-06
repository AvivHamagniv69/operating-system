#pragma once
#include "stdint.h"

#define WIDTH 80
#define HEIGHT 25

#define COLOR_BLACK 0
#define COLOR_GRAY 7

void print_char(char c);
void print_string(const char* const s, const uint32_t len);
void scroll_up(void);
void reset(void);
void print(const char* s, const uint16_t color);