#pragma once
#include <stdint.h>
#include <stddef.h>

uint64_t ceil_div(uint64_t a, uint64_t b);

void *memcpy(void *restrict dest, const void *restrict src, size_t n);

void *memset(void *s, int c, size_t n);

void *memmove(void *dest, const void *src, size_t n);

int memcmp(const void *s1, const void *s2, size_t n);

void hcf(void);

void outb(uint16_t port, uint8_t val);

uint8_t inb(uint16_t port);