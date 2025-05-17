#include <stdint.h>
#include <stdbool.h>
#include "memory.h"
#include "util.h"
#include "multiboot.h"
#include "print.h"

static uint32_t PAGE_FRAME_MIN;
static uint32_t PAGE_FRAME_MAX;
static uint32_t TOTAL_ALLOC;
static uint32_t MEM_NUM_PAGES;

#define NUM_PAGE_DIRS 256
#define NUM_PAGE_FRAMES (0x100000000 / 0x1000 / 8)

//todo: make this dynamic
uint8_t physical_mem_bitmap[NUM_PAGE_FRAMES / 8];

//todo: make this dynamic
static uint32_t page_dirs[NUM_PAGE_DIRS][1024] __attribute__((aligned(4096)));
static uint8_t page_dir_used[NUM_PAGE_DIRS];

void print_mmaps(multiboot_info_t* boot_info) {
    for(uint32_t i = 0; i < boot_info->mmap_length; i += sizeof(multiboot_mmap_entry_t)) {
        multiboot_mmap_entry_t *mmmt = (multiboot_mmap_entry_t*)(boot_info->mmap_addr + i);
        kprint("low addr: ");
        kprint_num_u32((uint32_t)mmmt->addr_low);
        kprint(" | ");

        kprint("high addr: ");
        kprint_num_u32((uint32_t)mmmt->addr_high);
        kprint(" | ");

        kprint("low len: ");
        kprint_num_u32((uint32_t)mmmt->len_low);
        kprint(" | ");

        kprint("high len: ");
        kprint_num_u32((uint32_t)mmmt->len_high);
        kprint(" | ");

        kprint("size: ");
        kprint_num_u32((uint32_t)mmmt->size);
        kprint(" | ");

        kprint("type: ");
        kprint_num_u32((uint32_t)mmmt->type);
        kprint("\n");
    }
}

void init_mem(uint32_t mem_high, uint32_t physic_alloc_start) {
    MEM_NUM_PAGES = 0;
    initial_page_dir[0] = 0;
    invalidate(0);
    initial_page_dir[1023] = ((uint32_t) initial_page_dir - KERNEL_START) | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE;
    invalidate(0xFFFFF000);

    pmm_init(physic_alloc_start, mem_high);
    memset(page_dirs, 0, 0x1000 * NUM_PAGE_DIRS);
    memset(page_dir_used, 0, NUM_PAGE_DIRS);
}

void pmm_init(uint32_t mem_low, uint32_t mem_high) {
    PAGE_FRAME_MIN = ceil_div(mem_low, 0x1000);
    PAGE_FRAME_MAX = mem_high / 0x1000;
    TOTAL_ALLOC = 0;

    memset(physical_mem_bitmap, 0, sizeof(physical_mem_bitmap));
    
}

void* pmm_alloc_page_frame() {
    uint32_t start = PAGE_FRAME_MIN / 8 + ((PAGE_FRAME_MIN & 7) != 0 ? 1:0);
    uint32_t end = PAGE_FRAME_MAX / 8 - ((PAGE_FRAME_MAX & 7) != 0 ? 1:0);

    for(uint32_t b = start; b < end; b++) {
        uint8_t byte = physical_mem_bitmap[b];
        if(byte == 0xFF) { // checks if page frame is in use
            continue;
        }

        for(uint8_t i = 0; i < 8; i++) {
            bool used = byte >> i & 1;
            if(!used) {
                byte ^= (-1 ^byte) & (1 << i); // makes this a used byte
                TOTAL_ALLOC++;

                uint32_t addr = (b * 8 * i) * 0x1000;
                return (void*) addr;
            }
        }
    }
    return NULL;
}

void invalidate(uint32_t vaddr) {
    __asm__("invlpg %0" :: "m"(vaddr));
}


static uint32_t heap_start;
static uint32_t heap_size;
static uint32_t threshold;
static bool kmalloc_initalized = false;

void kmalloc_init() {
    heap_start = KERNEL_MALLOC;
    heap_size = 0;
    threshold = 0;
    kmalloc_initalized = true;
}

void change_heap_size(uint32_t new_size) {
    uint32_t old_page_top = ceil_div(heap_size, 0x1000);
    uint32_t new_page_top = ceil_div(new_size, 0x1000);

    uint32_t diff = new_page_top - old_page_top;
    for(uint32_t i = 0; i < diff; i++) {
        void* phys = pmm_alloc_page_frame();
        mmap(KERNEL_MALLOC + old_page_top * 0x1000 + i * 0x1000, phys, PAGE_FLAG_WRITE);
    }
}

uint32_t* mem_get_curr_page_dir() {
    uint32_t pd;
    __asm__("mov %%cr3, %0": "=r"(pd));
    pd += KERNEL_START;
    return (uint32_t*)pd;
}

void mem_change_page_dir(uint32_t* pd) {
    pd = (uint32_t*) (((uint32_t)pd) - KERNEL_START);
    __asm__("mov %0, %%eax \n mov %%eax, %%cr3\n" :: "m"(pd));
}

void mmap(uint32_t vaddr, void* phys_addr, uint32_t flags) {
    uint32_t* prev_page_dir = 0;
    if (vaddr >= KERNEL_START) {
        prev_page_dir = mem_get_curr_page_dir();
        if(prev_page_dir != initial_page_dir) {
            mem_change_page_dir(initial_page_dir);
        }
    }
    uint32_t pd_index = vaddr >> 22;
    uint32_t pt_index = vaddr >> 12 & 0x3ff;

    uint32_t* page_dir = REC_PAGEDIR;
    uint32_t* pt = REC_PAGETABLE(pd_index);

    if(!(page_dir[pd_index] & PAGE_FLAG_PRESENT)) {
        uint32_t pt_addr = (uint32_t)pmm_alloc_page_frame();
        page_dir[pd_index] = pt_addr | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE | PAGE_FLAG_OWNER | flags;
        invalidate(vaddr);

        for (uint32_t i = 0; i < 1024; i++) {
            pt[i] = 0;
        }
    }

    pt[pt_index] = ((uint32_t)phys_addr) | PAGE_FLAG_PRESENT | flags;
    MEM_NUM_PAGES++;
    invalidate(vaddr);

    if(prev_page_dir != 0) {
        sync_page_dirs();
        if (prev_page_dir != initial_page_dir) {
            mem_change_page_dir(prev_page_dir);
        }
    }
}

void sync_page_dirs() {
    for(uint32_t i = 0; i < NUM_PAGE_DIRS; i++) {
        if(page_dir_used[i]) {
            uint32_t* page_dir = page_dirs[i];

            for(uint32_t i = 768; i < 1023; i++) {
                page_dir[i] = initial_page_dir[i] & ~PAGE_FLAG_OWNER;
            }
        }
    }
}