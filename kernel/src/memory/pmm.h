#pragma once

#define PAGE_SIZE 4096ULL

void pmm_free(uint64_t page);

uint64_t pmm_alloc();

void pmm_init();