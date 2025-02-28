#pragma once
#include <stdint.h>
#include <stddef.h>
#include "util.h"

struct GdtEntry __attribute__((packed));
typedef struct GdtEntry GdtEntry; 

struct GdtPtr __attribute__((packed));
typedef struct GdtPtr GdtPtr;

struct TssEntry __attribute__((packed));
typedef struct TssEntry TssEntry;

extern void gdt_flush(uint32_t gdt_ptr);
extern void tss_flush(void);

void init_gdt();

GdtEntry set_gdt_gate(uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);

GdtEntry write_tss(uint16_t ss0, uint32_t esp0);