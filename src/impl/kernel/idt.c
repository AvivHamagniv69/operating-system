#include <stdint.h>
#include "idt.h"
#include "util.h"
#include "print.h"

IdtEntry idt_entries[256] = {0};
IdtPtr idt_ptr;

void init_idt(void) {
    idt_ptr.limit = sizeof(IdtEntry) * 256 - 1;
    idt_ptr.base = (uint32_t) &idt_entries;

    outb(0x20, 0x11);
    outb(0xa0, 0x11);

    outb(0x21, 0x20);
    outb(0xa1, 0x28);

    outb(0x21, 0x04);
    outb(0xa1, 0x02);

    outb(0x21, 0x01);
    outb(0xa1, 0x01);

    outb(0x21, 0x0);
    outb(0xa1, 0x0);

    idt_entries[0] = set_idt_gate((uint32_t) isr0, 0x08, 0x8e);
    idt_entries[1] = set_idt_gate((uint32_t) isr1, 0x08, 0x8e);
    idt_entries[2] = set_idt_gate((uint32_t) isr2, 0x08, 0x8e);
    idt_entries[3] = set_idt_gate((uint32_t) isr3, 0x08, 0x8e);
    idt_entries[4] = set_idt_gate((uint32_t) isr4, 0x08, 0x8e);
    idt_entries[5] = set_idt_gate((uint32_t) isr5, 0x08, 0x8e);
    idt_entries[6] = set_idt_gate((uint32_t) isr6, 0x08, 0x8e);
    idt_entries[7] = set_idt_gate((uint32_t) isr7, 0x08, 0x8e);
    idt_entries[8] = set_idt_gate((uint32_t) isr8, 0x08, 0x8e);
    idt_entries[9] = set_idt_gate((uint32_t) isr9, 0x08, 0x8e);
    idt_entries[10] = set_idt_gate((uint32_t) isr10, 0x08, 0x8e);
    idt_entries[11] = set_idt_gate((uint32_t) isr11, 0x08, 0x8e);
    idt_entries[12] = set_idt_gate((uint32_t) isr12, 0x08, 0x8e);
    idt_entries[13] = set_idt_gate((uint32_t) isr13, 0x08, 0x8e);
    idt_entries[14] = set_idt_gate((uint32_t) isr14, 0x08, 0x8e);
    idt_entries[15] = set_idt_gate((uint32_t) isr15, 0x08, 0x8e);
    idt_entries[16] = set_idt_gate((uint32_t) isr16, 0x08, 0x8e);
    idt_entries[17] = set_idt_gate((uint32_t) isr17, 0x08, 0x8e);
    idt_entries[18] = set_idt_gate((uint32_t) isr18, 0x08, 0x8e);
    idt_entries[19] = set_idt_gate((uint32_t) isr19, 0x08, 0x8e);
    idt_entries[20] = set_idt_gate((uint32_t) isr20, 0x08, 0x8e);
    idt_entries[21] = set_idt_gate((uint32_t) isr21, 0x08, 0x8e);
    idt_entries[22] = set_idt_gate((uint32_t) isr22, 0x08, 0x8e);
    idt_entries[23] = set_idt_gate((uint32_t) isr23, 0x08, 0x8e);
    idt_entries[24] = set_idt_gate((uint32_t) isr24, 0x08, 0x8e);
    idt_entries[25] = set_idt_gate((uint32_t) isr25, 0x08, 0x8e);
    idt_entries[26] = set_idt_gate((uint32_t) isr26, 0x08, 0x8e);
    idt_entries[27] = set_idt_gate((uint32_t) isr27, 0x08, 0x8e);
    idt_entries[28] = set_idt_gate((uint32_t) isr28, 0x08, 0x8e);
    idt_entries[29] = set_idt_gate((uint32_t) isr29, 0x08, 0x8e);
    idt_entries[30] = set_idt_gate((uint32_t) isr30, 0x08, 0x8e);
    idt_entries[31] = set_idt_gate((uint32_t) isr31, 0x08, 0x8e);

    idt_entries[32] = set_idt_gate((uint32_t) irq0, 0x08, 0x8e);
    idt_entries[33] = set_idt_gate((uint32_t) irq1, 0x08, 0x8e);
    idt_entries[34] = set_idt_gate((uint32_t) irq2, 0x08, 0x8e);
    idt_entries[35] = set_idt_gate((uint32_t) irq3, 0x08, 0x8e);
    idt_entries[36] = set_idt_gate((uint32_t) irq4, 0x08, 0x8e);
    idt_entries[37] = set_idt_gate((uint32_t) irq5, 0x08, 0x8e);
    idt_entries[38] = set_idt_gate((uint32_t) irq6, 0x08, 0x8e);
    idt_entries[39] = set_idt_gate((uint32_t) irq7, 0x08, 0x8e);
    idt_entries[40] = set_idt_gate((uint32_t) irq8, 0x08, 0x8e);
    idt_entries[41] = set_idt_gate((uint32_t) irq9, 0x08, 0x8e);
    idt_entries[42] = set_idt_gate((uint32_t) irq10, 0x08, 0x8e);
    idt_entries[43] = set_idt_gate((uint32_t) irq11, 0x08, 0x8e);
    idt_entries[44] = set_idt_gate((uint32_t) irq12, 0x08, 0x8e);
    idt_entries[45] = set_idt_gate((uint32_t) irq13, 0x08, 0x8e);
    idt_entries[46] = set_idt_gate((uint32_t) irq14, 0x08, 0x8e);
    idt_entries[47] = set_idt_gate((uint32_t) irq15, 0x08, 0x8e);

    idt_entries[128] = set_idt_gate((uint32_t) isr128, 0x08, 0x8e);
    idt_entries[177] = set_idt_gate((uint32_t) isr177, 0x08, 0x8e);

    idt_flush((uint32_t) &idt_ptr);
}

IdtEntry set_idt_gate(uint32_t base, uint16_t sel, uint8_t flags) {
    IdtEntry entry;
    entry.base_low = base & 0xffff;
    entry.base_high = (base >> 16) & 0xffff;
    entry.sel = sel;
    entry.always0 = 0;
    entry.flags = flags | 0x60;

    return entry;
}

void isr_handler(InterruptRegisters* regs) {
    static char* exception_messages[] = {
        "division by zero",
        "debug",
        "non maskable interrupt",
        "breakpoint",
        "into detected overflow",
        "out of bounds",
        "invalid opcode",
        "no coprocessor",
        "double fault",
        "coprocessor segment overrun",
        "bad tss",
        "segment not present",
        "stack fault",
        "general protection fault",
        "page fault",
        "unknown interrupt",
        "coprocessor fault",
        "alignment check",
        "machine check",
        "reserved",
        "reserved",
        "reserved",
        "reserved",
        "reserved",
        "reserved",
        "reserved",
        "reserved",
        "reserved",
        "reserved",
        "reserved",
        "reserved",
        "reserved",
    };

    if(regs->int_no < 32) {
        kprint(exception_messages[regs->int_no]);
        kprint("exception! sytem halted");
        for(;;);
    }
}

void* irq_routines[16] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
};

void irq_install_handler(uint32_t irq, void (*handler)(InterruptRegisters* r)) {
    irq_routines[irq] = handler;
}

void irq_uninstall_handler(uint32_t irq) {
    irq_routines[irq] = 0;
}

void irq_handler(InterruptRegisters* regs) {
    void (*handler)(InterruptRegisters* regs);
    handler = irq_routines[regs->int_no - 32];

    if(handler) {
        handler(regs);
    }

    if(regs->int_no >= 40) {
        outb(0xa0, 0b100000);
    }

    outb(0x20, 0b100000);
}