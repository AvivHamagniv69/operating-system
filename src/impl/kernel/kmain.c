#include <stdint.h>
#include <stddef.h>
#include "print.h"
#include "util.h"
#include "gdt.h"
#include "idt.h"
#include "pit.h"
#include "../drivers/ps2.h"
#include "mem.h"
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

    init_mem(boot_info);
    kprint("52\% of the crime");

    for(;;);
    return;
}