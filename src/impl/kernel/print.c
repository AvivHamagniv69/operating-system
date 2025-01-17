#include <stdint.h>
#include <stddef.h>
#include "print.h"

typedef enum Color {
    COLOR_BLACK = 0,
    COLOR_WHITE = 15,
} Color;

static uint16_t* const vmemory = (uint16_t*) 0xb8000;
static size_t curr_column = 0;
static size_t curr_row = 0;
static size_t column_amt = 80;
static size_t row_amt = 25;
static const uint8_t default_color = COLOR_WHITE | COLOR_BLACK << 4;
static uint8_t curr_color = default_color;

void print_char(const char c) {
    switch(c) {
        default:
            vmemory[column_amt * curr_row + curr_column] = c | curr_color << 8;
            curr_column++;            

            if(curr_column >= column_amt && curr_row >= row_amt) {
                // TODO
                curr_column = 0;
                curr_row = 0;
            }
            else if(curr_column >= column_amt) {
                curr_column = 0;
                curr_row++;
            }
            break;
    }
}

void clear_screen() {
    for(size_t y = 0; y < row_amt; y++) {
        for(size_t x = 0; x < column_amt; x++) {
            vmemory[column_amt * y + x] = 0 | curr_color << 8;
        }
    }
}

void print_str(const char* const str, const size_t len) {
    for(size_t i = 0; i < len; i++) {
        print_char(str[i]);
    }
}

void set_color(const uint8_t foreground, const uint8_t background) {
    curr_color = foreground | background << 4;
}