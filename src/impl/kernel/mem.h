#pragma once
#include <stdint.h>

void init_mem(uint32_t memHigh, uint32_t physicalAllocStart);
void* kmalloc(uint32_t size);