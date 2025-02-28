#pragma once
#include <stdint.h>
#include <stddef.h>

typedef enum Color Color;

void print_char(const char c);
void clear_screen();
void print_str(const char* str);
void set_color(const uint8_t foreground, const uint8_t background);