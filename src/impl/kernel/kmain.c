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

void kmain(uint32_t addr) {
    multiboot_info_t* boot_info = (multiboot_info_t*)addr;
    clear_screen();

    init_gdt();
    init_idt();
    init_pit();
    init_keyboard();
    kprint("hello world\n");
    // kprint_num8(8);

    /*if(magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        kprint("Invalid magic number: ");
        kprint_num_u32(magic);
        for(;;);
    }*/

    init_mem(boot_info);


    for(;;);
    return;
}