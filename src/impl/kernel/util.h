#pragma once
#include <stddef.h>

struct InterruptRegisters {
    uint32_t cr2;
    uint32_t ds;

    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;

    uint32_t int_no;
    uint32_t err_code;

    uint32_t eip;
    uint32_t csm;
    uint32_t eflags;
    uint32_t useresp;
    uint32_t ss;
};

typedef struct InterruptRegisters InterruptRegisters;

void *memcpy(void *dest, const void *src, size_t n);

void *memset(void *s, int c, size_t n);

void *memmove(void *dest, const void *src, size_t n);

int memcmp(const void *s1, const void *s2, size_t n);

void outb(uint16_t port, uint8_t val);

uint8_t inb(uint16_t port);