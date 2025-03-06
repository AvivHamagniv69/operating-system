#include <stdint.h>
#include <stddef.h>
#include "print.h"
#include "util.h"
#include "gdt.h"
#include "idt.h"
#include "pit.h"

void ps2(InterruptRegisters* regs) {
    uint8_t a = inb(0x60);
    if(a != 0) {
        kprint_num8(a);
        kprint("\n");
    }
}

void init_keyboard(void) {
    irq_install_handler(1, &ps2);    
}

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