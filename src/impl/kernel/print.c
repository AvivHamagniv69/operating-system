#include <stdint.h>
#include <stddef.h>
#include "util.h"
#include "print.h"

static uint16_t* const vmemory = (uint16_t*) 0xb8000;
static size_t curr_column = 0;
static size_t curr_row = 0;
static size_t column_amt = 80;
static size_t row_amt = 25;
static const uint16_t default_color = ((uint8_t)COLOR_WHITE | (uint8_t)COLOR_BLACK << 4) << 8;
static uint16_t curr_color = default_color;

void scroll_up(){
    for (uint16_t y = 0; y < row_amt; y++){
        for (uint16_t x = 0; x < column_amt; x++){
            vmemory[(y-1) * column_amt + x] = vmemory[y*column_amt+x];
        }
    }

    for (uint16_t x = 0; x < column_amt; x++){
        vmemory[(row_amt-1) * column_amt + x] = ' ' | curr_color;
    }
}

static inline void pnewline(void) {
    if(curr_row < row_amt - 1) {
        curr_row++;
        curr_column = 0;
    }
    else {
        scroll_up();
        curr_column = 0;
    }
}

void pchar(const char c) {
    vmemory[column_amt * curr_row + curr_column] = c | curr_color;
    curr_column++;            

    if(curr_column >= column_amt && curr_row >= row_amt) {
        // TODO
        curr_column = 0;
        curr_row = 0;
    }
    else if(curr_column >= column_amt) {
        pnewline();
    }
}

void clear_screen() {
    curr_row = 0;
    curr_column = 0;
    curr_color = default_color;

    for (uint16_t y = 0; y < row_amt; y++){
        for (uint16_t x = 0; x < column_amt; x++){
            vmemory[y * column_amt + x] = ' ' | default_color;
        }
    }
}

void kprint(const char* str) {
    while(*str){
        switch(*str){
            case '\n':
                pnewline();
                break;
            case '\r':
                curr_column = 0;
                break;
            case '\b':
                if (curr_column == 0 && curr_row != 0){
                    curr_row--;
                    curr_column = column_amt;
                }
                vmemory[curr_row * column_amt + (--curr_column)] = ' ' | curr_color;
                break;
            case '\t':
                if (curr_column == column_amt){
                    pnewline();
                }
                uint16_t tabLen = 4 - (curr_column % 4);
                while (tabLen != 0){
                    vmemory[curr_row * column_amt + (curr_column++)] = ' ' | curr_color;
                    tabLen--;
                }
                break;
            default:
                if (curr_column == column_amt){
                    pnewline();
                }

                vmemory[curr_row * column_amt + (curr_column++)] = *str | curr_color;
                break;
        }
        str++;
    }

}

void kprint_num8(uint8_t num) {
    char buf[4] = {-1, -1, -1, 0};
    int i = 2;
    while(num > 0) {
        buf[i] = '0'+(num%10);
        num /= 10;
        i--;
    }
    i = 0;
    while(i < 3 && buf[i] == -1) {
        i++;
    }
    kprint(&buf[i]);
}

void set_color(const uint8_t foreground, const uint8_t background) {
    curr_color = (foreground | background << 4) << 8;
}