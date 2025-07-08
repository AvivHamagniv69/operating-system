#pragma once
#include <stdint.h>

void paging_init();

void* kmalloc(uint64_t size);

void kfree(void* addr);