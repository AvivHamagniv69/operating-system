#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct {
	uint16_t isr_low;      // The lower 16 bits of the ISR's address
	uint16_t kernel_cs;    // The GDT segment selector that the CPU will load into CS before calling the ISR
	uint8_t ist;          // The IST in the TSS that the CPU will load into RSP; set to zero for now
	uint8_t attributes;   // Type and attributes; see the IDT page
	uint16_t isr_mid;      // The higher 16 bits of the lower 32 bits of the ISR's address
	uint32_t isr_high;     // The higher 32 bits of the ISR's address
	uint32_t reserved;     // Set to zero
} __attribute__((packed)) IdtEntry;


typedef struct {
	uint16_t	limit;
	uint64_t	base;
} __attribute__((packed)) Idtr;

typedef struct {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rdx;      
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;

    uint64_t vector_number;
    uint64_t error_code;

    uint64_t iret_rip;
    uint64_t iret_cs;
    uint64_t iret_flags;
    uint64_t iret_rsp;
    uint64_t iret_ss;
} __attribute__((packed)) Regs;

extern bool vectors[256];

void exception_handler(Regs* regs);

void idt_set_descriptor(uint8_t vector, void* isr, uint8_t flags);

void idt_init();

void irq_install_handler(uint8_t irq, void (*handler)(Regs* regs));

void irq_uninstall_handler(uint8_t irq);