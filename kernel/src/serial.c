#include <stdbool.h>
#include "limine.h"
#include "serial.h"
#include "util.h"

uint8_t serial_init() {
    outb(COM1 + 1, 0x00);    // Disable all interrupts
    outb(COM1 + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(COM1 + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    outb(COM1 + 1, 0x00);    //                  (hi byte)
    outb(COM1 + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(COM1 + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    outb(COM1 + 4, 0x0B);    // IRQs enabled, RTS/DSR set
    outb(COM1 + 4, 0x1E);    // Set in loopback mode, test the serial chip
    outb(COM1 + 0, 0xAE);    // Send a test byte

    // Check that we received the same test byte we sent
    if(inb(COM1 + 0) != 0xAE) {
        return 1;
    }

    // If serial is not faulty set it in normal operation mode:
    // not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled
    outb(COM1 + 4, 0x0F);
    return 0;
}

static inline bool is_transmit_empty(void) {
	return inb(COM1 + 5) & 0x20;
}
 
void write_serial(char a) {
	while (!is_transmit_empty());
 
	outb(COM1, a);
}

void serial_log(char* str) {
    while(*str != '\0') {
        write_serial(*str);
        str++;
    }
}

static void serial_log_num_unsigned_recursive(const uint64_t num) {
    if(num == 0) {
        return;
    }
    serial_log_num_unsigned_recursive(num / 10);
    write_serial('0' + (num % 10));
}

void serial_log_num_unsigned(uint64_t num) {
    if(num == 0) {
        write_serial('0');
        return;
    }
    serial_log_num_unsigned_recursive(num);
}

static void serial_log_num_signed_recursive(const int64_t num) {
    if(num == 0) {
        write_serial('-');
        return;
    }
    serial_log_num_signed_recursive(num / 10);
    write_serial('0' + (num % 10));
}

void serial_log_num_signed(const int64_t num) {
    if(num == 0) {
        write_serial('0');
        return;
    }
    serial_log_num_signed_recursive(num);
}