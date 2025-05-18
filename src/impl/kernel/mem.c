#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "multiboot.h"
#include "util.h"
#include "mem.h"

#define KERNEL_START 0xC0000000

#define RW_FLAG (1 << 1)
#define CANT_RW_FLAG (0 << 1)
#define USER_FLAG (1 << 2)
#define SUPERVISOR_FLAG (0 << 2)
#define PAGE_SIZE_4K (0 << 7)

// todo: dynamic, mark unavailble pages
#define PAGE_FRAME_AMT 32768
#define PAGE_SIZE 0x1000
#define BITS_PER_ELEMENT 32
static uint32_t mem_bitmap[PAGE_FRAME_AMT] __attribute__((aligned(PAGE_SIZE)));

#define PAGE_DIR_AMT 1024
#define PAGE_TABLE_AMT 1024
static uint32_t page_dir[PAGE_DIR_AMT][PAGE_TABLE_AMT] __attribute__((aligned(PAGE_SIZE)));

extern uint32_t initial_page_dir[PAGE_DIR_AMT] __attribute__((aligned(PAGE_SIZE)));

static inline BitMapPtr phys_addr_to_map_ptr(uint32_t addr) {
    BitMapPtr a = {index: addr/0x1000, bit: addr%32};
    return a;
}

static inline void* map_ptr_to_phys_addr(BitMapPtr ptr) {
    uint32_t frame_number = ptr.index * BITS_PER_ELEMENT + ptr.bit;
    return (void*)(frame_number * PAGE_SIZE);
}

static inline uint8_t get_bit(BitMapPtr ptr) {
    return (mem_bitmap[ptr.index] >> (ptr.bit - 1)) & 0x00000001;
}

static inline void set_bit(BitMapPtr ptr) {
    mem_bitmap[ptr.index] |= 1U << ptr.bit;
}

static inline void free_bit(BitMapPtr ptr) {
    mem_bitmap[ptr.index] &= ~(1U << ptr.bit);
}

static inline bool incr_ptr(BitMapPtr ptr) {
    ptr.bit++;
    if(ptr.bit < BITS_PER_ELEMENT) {
        return true;
    }
    ptr.index++;
    ptr.bit = 0;
    if(ptr.index < PAGE_FRAME_AMT) {
        return true;
    }
    // out of bounds return error
    return false;
}

static bool mark_unavailable_mem(multiboot_info_t* boot_info) {
    for(uint32_t i = 0; i < (boot_info)->mmap_length;) {
        multiboot_mmap_entry_t *mmmt = (multiboot_mmap_entry_t*)(boot_info->mmap_addr + i);
        uint32_t type = mmmt->type;
        if(type == 1) {
            continue;
        }
        uint32_t start = mmmt->addr_low & ~0xFFF;
        uint32_t end = (mmmt->addr_low + mmmt->len_low + 0xFFF) & ~0xFFF;

        for(uint32_t j = start; j < end; j += 0x1000) {
            set_bit(phys_addr_to_map_ptr(j));
        }
        i += mmmt->size + sizeof(mmmt->size);
    }
    return false;
}

static uint32_t* mem_get_curr_page_dir() {
    uint32_t pd;
    __asm__("mov %%cr3, %0": "=r"(pd));
    pd += KERNEL_START;
    uint32_t* _pd = (uint32_t*) pd;
    return _pd;
}

static void* get_physaddr(void *vaddr) {
    uint32_t pdindex = (uint32_t)vaddr >> 12;
    uint32_t ptindex = (uint32_t)vaddr >> 12 & 0x03FF;

    uint32_t* pd = mem_get_curr_page_dir();
    //todo: Here you need to check whether the PD entry is present.

    uint32_t* pt = (uint32_t*) pd[pdindex];
    //todo: Here you need to check whether the PT entry is present.

    return (void *)((pt[ptindex] & ~0xFFF) + ((uint32_t)vaddr & 0xFFF));
}

static void mem_change_page_dir(uint32_t* pd) {
    pd = (uint32_t*) (((uint32_t)pd) - KERNEL_START);
    __asm__("mov %0, %%eax \n mov %%eax, %%cr3\n" :: "m"(pd));
}

static void sync_page_dirs() {
    for (uint32_t i = 0; i < PAGE_DIR_AMT; i++){
        if (initial_page_dir[i] & 0x1){
            uint32_t* pd = page_dir[i];

            for (int i = 768; i < 1023; i++){
                pd[i] = initial_page_dir[i];
            }
        }
    }
}

static void invalidate(uint32_t vaddr) {
    __asm__("invlpg %0" :: "m"(vaddr));
}

static void map_page(void *physaddr, void *vaddr, int32_t flags) {
    // Make sure that both addresses are page-aligned.

    uint32_t pdindex = (uint32_t)vaddr >> 12;
    uint32_t ptindex = (uint32_t)vaddr >> 12 & 0x03FF;

    uint32_t *pd = mem_get_curr_page_dir();
    // Here you need to check whether the PD entry is present.
    // When it is not present, you need to create a new empty PT and
    // adjust the PDE accordingly.

    uint32_t *pt = (uint32_t*) pd[pdindex];
    // Here you need to check whether the PT entry is present.
    // When it is, then there is already a mapping present. What do you do now?

    pt[ptindex] = ((uint32_t)physaddr) | (flags & 0xFFF) | 0x01; // Present

    // Now you need to flush the entry in the TLB
    // or you might not notice the change.
    invalidate((uint32_t) vaddr);
}

static inline bool is_page_present(uint32_t i, uint32_t j) {
    return page_dir[i][j] & 0x01;
}

void init_mem(multiboot_info_t* boot_info) {
    initial_page_dir[0] = 0;
    invalidate(0);
    initial_page_dir[1023] = ((uint32_t) initial_page_dir - KERNEL_START) | 0x1 | USER_FLAG | RW_FLAG;
    mark_unavailable_mem(boot_info);
    invalidate(0xFFFFF000);

    memset(page_dir, 0, 0x1000 * PAGE_DIR_AMT);

    sync_page_dirs();
    uint32_t* pd = (uint32_t*) (((uint32_t)page_dir) - KERNEL_START);
    __asm__("mov %0, %%eax \n mov %%eax, %%cr3\n" :: "m"(pd));
}

void* kmalloc(uint32_t size) {
    size = ceil_div(size, PAGE_SIZE);
    uint32_t size_found = 0;
    uint32_t size_remaining = size;
    uint32_t i = 0;
    uint32_t j = 0;
    uint32_t mem_start_i = i;
    uint32_t mem_start_j = j;
    bool start_over = false;
    while(i < PAGE_DIR_AMT && size_found < size) {
        if(((uint32_t)page_dir[i]) & 0x01) {
            size_found = 0;
            size_remaining = size;
            mem_start_i = i;
            mem_start_j = j;
            start_over = false;

            i++;
            j++;
            continue;
        }

        if(start_over) {
            size_found = 0;
            size_remaining = size;
            mem_start_i = i;
            mem_start_j = j;
            start_over = false;
        }
        while(j < PAGE_TABLE_AMT && size_found < size) {
            if(is_page_present(i, j)) {
                start_over = true;
            }
            else {
                size_found += PAGE_SIZE;
                size_remaining -= PAGE_SIZE;
            }
            j++;
            if(start_over && j < PAGE_TABLE_AMT) {
                size_found = 0;
                size_remaining = size;
                mem_start_i = i;
                mem_start_j = j;
                start_over = false;
            }
        }
        i++;
        j = 0;
    }
    if(size_found != size) {
        return NULL;
    }

    void* address_start = (void*)page_dir[mem_start_i][mem_start_j];
    uint32_t mem_end_i = i;
    uint32_t mem_end_j = j;
    BitMapPtr ptr = {index: 0, bit: 0};
    i = mem_start_i;
    j = 0;
    while(i < mem_end_i - 1 && ptr.index < PAGE_FRAME_AMT) {
        while(j < PAGE_TABLE_AMT) { //todo
            if(get_bit(ptr) == 0) {
                map_page(map_ptr_to_phys_addr(ptr), (void*)page_dir[i][j], RW_FLAG | USER_FLAG);
                set_bit(ptr);
                j++;
            }
            bool check = incr_ptr(ptr);
            if(!check) {
                // TODO: free mem properly
                return NULL;
            }
        }
        i++;
        j = 0;
    }

    j = 0;
    while(j < mem_end_j) {
        if(get_bit(ptr) == 0) {
            map_page(map_ptr_to_phys_addr(ptr), (void*)page_dir[i][j], RW_FLAG | USER_FLAG);
            set_bit(ptr);
            j++;
        }
        bool check = incr_ptr(ptr);
        if(!check) {
            // TODO: free mem properly
            return NULL;
        }
    }

    return address_start;
}

void* kfree(void* vaddr) {
    return NULL;
}