#include <stdint.h>
#include "memory.h"
#include "util.h"
#include "multiboot.h"
#include "print.h"

void init_mem(multiboot_info_t* boot_info) {
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