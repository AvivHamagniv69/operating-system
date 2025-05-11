#include <stdint.h>
#include "memory.h"
#include "util.h"
#include "multiboot2.h"
#include "print.h"

void init_mem(multiboot_info_t* boot_info) {
    for(uint32_t i = 0; i < boot_info->mmap_length; i += 1) {
        multiboot_mmap_entry_t *mmmt = (multiboot_mmap_entry_t*)(boot_info->mmap_addr);
        kprint("low addr: ");
        kprint_num_u32((uint32_t)mmmt->addr_low);
        kprint("\n");

        kprint("high addr: ");
        kprint_num_u32((uint32_t)mmmt->addr_high);
        kprint("\n");

        kprint("low len: ");
        kprint_num_u32((uint32_t)mmmt->len_low);
        kprint("\n");

        kprint("high len: ");
        kprint_num_u32((uint32_t)mmmt->len_high);
        kprint("\n");

        kprint("size: ");
        kprint_num_u32((uint32_t)mmmt->size);
        kprint("\n");

        kprint("type: ");
        kprint_num_u32((uint32_t)mmmt->type);
        kprint("\n\n");
    }
}