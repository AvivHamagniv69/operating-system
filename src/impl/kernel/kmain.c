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


    init_mem(boot_info->mem_lower * 1024, boot_info->mem_upper * 1024);
    char* a = (char*) kmalloc(10);
    a[0] = 'a';
    a[1] = 'v';
    a[2] = 'i';
    a[3] = 'v';
    a[4] = '\n';
    a[5] = '\0';
    uint32_t* b = (uint32_t*) kmalloc(sizeof(uint32_t));
    *b = 69;
    kprint("num: ");
    kprint_num_u32(*b);
    kprint("\n");
    kprint_num_u8(a[0]);

    kprint("52\% of the crime\n");

    kprint("53\% of the crime");


    for(;;);
    return;
}