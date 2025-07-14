#include <stdint.h>
#include "util.h"
#include "idt.h"
#include "print/print.h"
#include "serial.h"
#include "ps2.h"

#define CAPS_LOCK_PRESSED 0x3a
#define LEFT_SHIFT_PRESSED 0x2a
#define RIGHT_SHIFT_PRESSED 0x36
#define LEFT_SHIFT_RELEASED 0xaa
#define RIGHT_SHIFT_RELEASED 0xb6
#define LEFT_CONTROL_PRESSED 0x1d
#define KEYPAD_PRESSED 0x37
#define LEFT_ALT_PRESSED 0x38

void ps2(Regs* regs) {
    uint8_t a = inb(0x60);
    
    static char lowercase_table[] = {
        '1',
        '2',
        '3',
        '4',
        '5',
        '6',
        '7',
        '8',
        '9',
        '0',
        '-',
        '=',
        '\b',
        '\t',
        'q',
        'w',
        'e',
        'r',
        't',
        'y',
        'u',
        'i',
        'o',
        'p',
        '[',
        ']',
        '\n',
        '\0', // left control pressed
        'a',
        's',
        'd',
        'f',
        'g',
        'h',
        'j',
        'k',
        'l',
        ';',
        '\'',
        '`',
        '\0', // left shift pressed
        '\\',
        'z',
        'x',
        'c',
        'v',
        'b',
        'n',
        'm',
        ',',
        '.',
        '/',
        '\0', // right shift pressed
        '\0', // keypad pressed
        '\0', // left alt pressed
        ' '
    };

    static char uppercase_table[] = {
        '1',
        '2',
        '3',
        '4',
        '5',
        '6',
        '7',
        '8',
        '9',
        '0',
        '-',
        '=',
        '\b',
        '\t',
        'Q',
        'W',
        'E',
        'R',
        'T',
        'Y',
        'U',
        'I',
        'O',
        'P',
        '[',
        ']',
        '\n',
        '\0', // left control
        'A',
        'S',
        'D',
        'F',
        'G',
        'H',
        'J',
        'K',
        'L',
        ';',
        '\'',
        '`',
        '\0', // left shift
        '\\',
        'Z',
        'X',
        'C',
        'V',
        'B',
        'N',
        'M',
        ',',
        '.',
        '/',
        '\0', // right shift pressed
        '\0', // keypad pressed
        '\0', // left alt pressed
        ' '
    };

    static char shift_table[] = {
        '!',
        '@',
        '#',
        '$',
        '%',
        '^',
        '&',
        '*',
        '(',
        ')',
        '_',
        '+',
        '\b',
        '\t',
        'Q',
        'W',
        'E',
        'R',
        'T',
        'Y',
        'U',
        'I',
        'O',
        'P',
        '{',
        '}',
        '\n',
        '\0', // left control
        'A',
        'S',
        'D',
        'F',
        'G',
        'H',
        'J',
        'K',
        'L',
        ':',
        '\'',
        '~',
        '\0', // left shift
        '\\',
        'Z',
        'X',
        'C',
        'V',
        'B',
        'N',
        'M',
        '<',
        '>',
        '|',
        '\0', // right shift pressed
        '\0', // keypad pressed
        '\0', // left alt pressed
        ' '
    };

    static uint32_t table_len = sizeof(lowercase_table);
    static char* table = lowercase_table;
    
    if(a == 0) {
        return;
    }
    else if(
        a-2 < table_len &&
        a != LEFT_CONTROL_PRESSED &&
        a != LEFT_SHIFT_PRESSED &&
        a != RIGHT_SHIFT_PRESSED &&
        a != KEYPAD_PRESSED &&
        a != LEFT_ALT_PRESSED
    ) {
        write_serial((char)table[a-2]);
    }
    else if(a == CAPS_LOCK_PRESSED) {
        if(table == lowercase_table) {
            table = uppercase_table;
        }
        else {
            table = lowercase_table;
        }
    }
    else if(a == LEFT_SHIFT_PRESSED || a == RIGHT_SHIFT_PRESSED) {
        table = shift_table; // todo
    }
    else if(a == LEFT_SHIFT_RELEASED || a == RIGHT_SHIFT_RELEASED) {
        table = lowercase_table; // todo
    }
}

void init_keyboard(void) {
    irq_install_handler(1, &ps2);    
}