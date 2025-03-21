#include <stdint.h>
#include <stddef.h>
#include "print.h"
#include "util.h"
#include "gdt.h"
#include "idt.h"
#include "pit.h"
#include "../drivers/ps2.h"
#include "memory.h"

extern uint32_t endkernel;

void kmain() {
    clear_screen();

    init_gdt();
    init_idt();
    init_pit();
    init_keyboard();
    kprint("hello world\n");
    // kprint_num8(8);


    for(;;);
    return;
}