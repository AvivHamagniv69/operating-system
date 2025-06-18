#include "idt.h"
#include "util.h"
#include "limine.h"
#include "serial.h"

extern uint64_t* isr_stub_table[];

static IdtEntry idt[256] __attribute__((aligned(0x10))) = {0}; // Create an array of IDT entries; aligned for performance
static Idtr idtr;
bool vectors[256] = {0};

static void* irq_routines[256] = {0};

void exception_handler() {
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
    /*if(regs->vector_number < 32) {
        serial_log(exception_messages[regs->vector_number]);
        serial_log("\n");
        serial_log("regs:\n");
        serial_log("\n");
        serial_log("err_code: ");
        serial_log_num_unsigned(regs->error_code);
        hcf();
    }*/
    hcf();
}

void idt_set_descriptor(uint8_t vector, void* isr, uint8_t flags) {
    IdtEntry* descriptor = &idt[vector];

    descriptor->isr_low        = (uint64_t)isr & 0xFFFF;
    descriptor->kernel_cs      = 0x08;
    descriptor->ist            = 0;
    descriptor->attributes     = flags;
    descriptor->isr_mid        = ((uint64_t)isr >> 16) & 0xFFFF;
    descriptor->isr_high       = ((uint64_t)isr >> 32) & 0xFFFFFFFF;
}

#define MASTER_CMD 0x20
#define MASTER_DATA 0x21
#define SLAVE_CMD 0xA0
#define SLAVE_DATA 0xA1

static void pic8259_remap() {
    outb(MASTER_CMD, 0x11);
    outb(SLAVE_CMD, 0x11);
    outb(MASTER_DATA, 0x20);
    outb(SLAVE_DATA, 0x28);
    outb(MASTER_DATA, 0x04);
    outb(SLAVE_DATA, 0x02);
    outb(MASTER_DATA, 0x01);
    outb(SLAVE_DATA, 0x01);
    outb(MASTER_DATA, 0x0);
    outb(SLAVE_DATA, 0x0);
}

void idt_init() {
    idtr.base = (uintptr_t)&idt[0];
    idtr.limit = (uint16_t)sizeof(IdtEntry) * 256;

    idt_set_descriptor(0, isr_stub_table[0], 0x8E);
    vectors[0] = true;

    for (uint8_t vector = 1; vector < 32; vector++) {
        idt_set_descriptor(vector, isr_stub_table[vector], 0x8E);
        vectors[vector] = true;
    }
    pic8259_remap();

    __asm__ volatile ("lidt %0" : : "m"(idtr)); // load the new IDT
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