#include "idt.h"
#include "util.h"
#include "limine.h"
#include "serial.h"

extern uint64_t* isr_stub_table[];
extern uint64_t* irq_stub_table[];

idt_entry_t idt[256] __attribute__((aligned(0x10))) = {0};
bool vectors[256] = {0};

static void* irq_routines[256] = {0};

void exception_handler(Regs* regs) {
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

    serial_log("excpetion! system halted\n");
    if(regs->vector_number < 32) {
        serial_log(exception_messages[regs->vector_number]);
        serial_log("\n");
        serial_log("regs:\n");
        serial_log("\n");
        serial_log("err_code: ");
        serial_log_num_unsigned(regs->error_code);
        hcf();
    }
}

void idt_set_descriptor(uint8_t vector, uint64_t isr, uint8_t flags) {
    idt_entry_t* entry = &idt[vector];
    entry->isr_low = isr & 0xFFFF;
    entry->isr_mid = (isr >> 16) & 0xFFFF;
    entry->isr_high = isr >> 32;
    //your code selector may be different!
    entry->kernel_cs = 0x8;
    entry->attributes = 0b1110 | ((flags & 0b11) << 5) |(1 << 7);
    //ist disabled
    entry->ist = 0;
    entry->reserved = 0;
}

static void irq_remap(void) {
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0x0);
    outb(0xA1, 0x0);
}

void idt_init() {
    idtr_t idtr = {0};
    idtr.limit = (uint16_t)(sizeof(idt) - 1);
    idtr.base = (uint64_t)idt;

    for (uint8_t vector = 0; vector < 32; vector++) {
        idt_set_descriptor(vector, (uint64_t)isr_stub_table[vector], 0x8E);
        vectors[vector] = true;
    }
    for (uint16_t vector = 32; vector < 256; vector++) {
        idt_set_descriptor(vector, (uint64_t)irq_stub_table[vector], 0x8E);
        vectors[vector] = true;
    }
    irq_remap();

    __asm__ volatile("lidt %0" : : "m"(idtr));
    __asm__ volatile ("sti"); // set the interrupt flag
}

void irq_install_handler(uint8_t irq, void (*handler)(Regs* regs)) {
    irq_routines[irq] = handler;
}

void irq_uninstall_handler(uint8_t irq) {
    irq_routines[irq] = 0;
}

void irq_handler(Regs* regs) {
    void (*handler)(Regs* regs) = irq_routines[regs->vector_number-32];
    if(handler) {
        handler(regs);
    }

    if(regs->vector_number-32 >= 40) {
        outb(0xA0, 0x20);
    }
    outb(0x20, 0x20);
}