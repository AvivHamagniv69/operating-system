#include <stdint.h>
#include <stddef.h>
#include "util.h"
#include "print.h"
#include "idt.h"

uint64_t ticks;
const uint32_t freq = 100;

void on_irq0(InterruptRegisters* regs) {
    ticks += 1;

    //kprint("timer ticked ");
}

void init_pit(void) {
    ticks = 0;
    irq_install_handler(0, &on_irq0);

    // 1.1931816666 MHz
    uint32_t divisor = 1193180/freq;

    outb(0x43, 0b00110110);
    outb(0x40, (uint8_t)(divisor & 0xff));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xff));
}