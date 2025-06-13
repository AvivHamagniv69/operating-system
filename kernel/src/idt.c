#include "idt.h"
#include "util.h"
#include "limine.h"
#include "serial.h"

idt_entry_t idt[256] __attribute__((aligned(0x10))) = {0};

bool vectors[256] = {0};

void exception_handler(regs* regs) {
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

    /*if(regs->int_no < 32) {
        kprint(exception_messages[regs->int_no]);
        kprint("\n");
        kprint("exception! sytem halted\n");
        kprint("regs:\n");
        kprint("cr2: ");
        kprint_num_u32(regs->cr2);
        kprint("\n");
        kprint("err_code: ");
        kprint_num_u32(regs->err_code);

        for(;;);
    }*/
    serial_log("exception!\n");
    hcf();
}

void idt_set_descriptor(uint8_t vector, void* isr, uint8_t flags) {
    uint64_t isr_addr = (uint64_t)isr;

    idt_entry_t* entry = &idt[vector];
    entry->isr_low = isr_addr & 0xFFFF;
    entry->isr_mid = (isr_addr >> 16) & 0xFFFF;
    entry->isr_high = isr_addr >> 32;
    //your code selector may be different!
    entry->kernel_cs = 0x8;
    entry->attributes = 0b1110 | ((flags & 0b11) << 5) |(1 << 7);
    //ist disabled
    entry->ist = 0;
    entry->reserved = 0;
}

void idt_init() {
    idtr_t idtr = {0};
    idtr.limit = (uint16_t)(sizeof(idt) - 1);
    idtr.base = (uint64_t)idt;

    for (uint8_t vector = 0; vector < 32; vector++) {
        idt_set_descriptor(vector, isr_stub_table[vector], 0x8E);
        vectors[vector] = true;
    }

    void* i = (void*)&idtr;
    __asm__ volatile("lidt %0" :: "m"(i));
    __asm__ volatile ("sti"); // set the interrupt flag
}