#pragma once
#include <stdint.h>
#include "multiboot.h"

#define KERNEL_START 0xC0000000
#define PAGE_FLAG_PRESENT (1 << 0)
#define PAGE_FLAG_WRITE (1 << 1)
#define PAGE_FLAG_OWNER (1 << 9)
#define KERNEL_MALLOC 0xD0000000
#define REC_PAGEDIR ((uint32_t*)0xFFFFF000)
#define REC_PAGETABLE(i) ((uint32_t*) (0xFFC00000 + ((i) << 12)))


extern uint32_t initial_page_dir[1024];

void print_mmaps(struct multiboot_info* boot_info);

void init_mem(uint32_t mem_high, uint32_t physic_alloc_start);

void pmm_init(uint32_t mem_low, uint32_t mem_high);

void invalidate(uint32_t vaddr);

void kmalloc_init();

void change_heap_size(uint32_t new_size);

uint32_t* mem_get_curr_page_dir();

void mem_change_page_dir(uint32_t* pd);

void mmap(uint32_t vaddr, void* phys_addr, uint32_t flags);

void sync_page_dirs();