#include <stdint.h>
#include <stddef.h>
#include "print.h"
#include "util.h"
#include "gdt.h"
#include "idt.h"
#include "pit.h"
#include "../drivers/ps2.h"
#include "memory.h"
#include "multiboot.h"

#define MULTIBOOT_BOOTLOADER_MAGIC 0x1BADB002

extern uint32_t endkernel;

void kmain(uint32_t magic, uint32_t addr) {
    multiboot_info_t* boot_info = (multiboot_info_t*)addr;
    clear_screen();

    init_gdt();
    init_idt();
    init_pit();
    init_keyboard();
    kprint("hello world\n");

    uint32_t mod1 = *(uint32_t*)(boot_info->mods_addr + 4);
    uint32_t physic_alloc_start = (mod1 + 0xFFF) & ~0xFFF;

    init_mem(boot_info->mem_upper * 1024, physic_alloc_start);
    kprint("52\% of the crime");

    for(;;);
    return;
}