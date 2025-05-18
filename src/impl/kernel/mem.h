#pragma once
#include "multiboot.h"

typedef struct BitMapPtr {
    uint32_t index;
    uint8_t bit;
} BitMapPtr;

void init_mem(multiboot_info_t* boot_info);
void* kmalloc(uint32_t size);
void* kfree(void* vaddr);