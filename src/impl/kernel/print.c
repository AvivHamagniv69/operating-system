#include <stdint.h>
#include <stddef.h>
#include "print.h"

typedef enum Color {
    COLOR_BLACK = 0,
    COLOR_WHITE = 15,
} Color;

static uint8_t* const vmemory = (uint8_t*) 0xb8000;
static size_t curr_col = 0;
static size_t curr_row = 0;
static size_t col_amt = 80;
static size_t row_amt = 25;
static const uint8_t default_color = COLOR_WHITE | COLOR_BLACK << 4;
static uint8_t curr_color = default_color;

void print_char(const char c) {
    switch(c) {
        default:
            vmemory[col_amt * curr_row + curr_col] = c | curr_color << 8;
            
            if(curr_col >= col_amt && curr_row >= row_amt) {
                // TODO
                curr_col = 0;
                curr_row = 0;
            }
            else if(curr_col >= col_amt) {
                curr_col = 0;
                curr_row++;
            }
            break;
    }
}

void clear_screen() {
    for(size_t y = 0; y < row_amt; y++) {
        for(size_t x = 0; x < col_amt; x++) {
            vmemory[col_amt * y + x] = 0 | curr_color << 8;
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