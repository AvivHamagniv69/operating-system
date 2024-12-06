#include "vga.h"

uint16_t column = 0;
uint16_t line = 0;
uint16_t* const vga = (uint16_t* const) 0xb8000;
const uint16_t default_color_background = COLOR_BLACK;
const uint16_t default_color_text = COLOR_GRAY;

void reset(void) {
    line = 0;
    column = 0;

    for(uint16_t x = 0; x < WIDTH; x++) {
        for(uint16_t y = 0; y < HEIGHT; y++) {
            vga[y * WIDTH + x] = ' ' | default_color_background;
        }
    }
}

void newline(void) {
    if(line < HEIGHT -1) {
        line++;
        column = 0;
    }
    else {
        scroll_up();
        column = 0;
    }
}

void scroll_up(void) {
    for(uint16_t y = 0; y < HEIGHT; y++) {
        for(uint16_t x = 0; x < WIDTH; x++) {
            vga[(y-1) * WIDTH + x] = vga[y * WIDTH + x];
        }
    }

    for(uint16_t x = 0; x < WIDTH; x++) {
        vga[(HEIGHT-1) * WIDTH + x] = ' ' | default_color_background;
    }
}

void print(const char* s, const uint16_t color) {
    while(*s != 0) {
        switch(*s) {
            case '\n':
                newline();
                break;
            default:
                if(column == WIDTH) {
                    newline();
                }
                vga[line * WIDTH + column] = *s | default_color_background; 
                break;
        }
        s++;
    }
}