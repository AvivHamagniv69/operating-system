#include <stdint.h>
#include <stddef.h>
#include "util.h"
#include "gdt.h"

GdtEntry gdt_entries[6];
GdtPtr gdt_ptr;
TssEntry tss_entry;

extern void gdt_flush(uint32_t gdt_ptr);
extern void tss_flush(void);

GdtEntry set_gdt_gate(uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    GdtEntry entry;
    
    entry.base_low = (uint16_t) (base & 0xffff);
    entry.base_middle = (uint8_t) ((base >> 16) & 0xff);
    entry.base_high = (uint8_t) ((base >> 24) & 0xff);

    entry.limit = (uint16_t) (limit & 0xffff);
    entry.flags = (uint8_t) ((limit >> 16) & 0x0f);
    entry.flags |= (uint8_t) (gran & 0xf0);

    entry.access = access;

    return entry;
}

void init_gdt() {
    gdt_ptr.limit = (sizeof(GdtEntry) * 6) - 1;
    gdt_ptr.base = (uint32_t) &gdt_entries;

    gdt_entries[0] = set_gdt_gate(0, 0, 0, 0);
    gdt_entries[1] = set_gdt_gate(0, 0xffffffff, 0x9a, 0xcf); // kernel code segment
    gdt_entries[2] = set_gdt_gate(0, 0xffffffff, 0x92, 0xcf); // kernel data segment
    gdt_entries[3] = set_gdt_gate(0, 0xffffffff, 0xfa, 0xcf); // user code segment
    gdt_entries[4] = set_gdt_gate(0, 0xffffffff, 0xf2, 0xcf); // user data segment
    gdt_entries[5] = write_tss(0x10, 0x0);
    // 0x10 is the offset for our gdt table

    gdt_flush((uint32_t)&gdt_ptr);
    tss_flush();
}

GdtEntry write_tss(uint16_t ss0, uint32_t esp0) {
    uint32_t base = (uint32_t) &tss_entry;
    uint32_t limit = base + sizeof(tss_entry);

    GdtEntry entry = set_gdt_gate(base, limit, 0xe9, 0x00);
    memset(&tss_entry, 0, sizeof(tss_entry));

    tss_entry.ss0 = ss0;
    tss_entry.esp0 = esp0;

    tss_entry.cs = 0x08 | 0x3;
    tss_entry.ss = 0x10 | 0x3;
    tss_entry.ds = 0x10 | 0x3;
    tss_entry.es = 0x10 | 0x3;
    tss_entry.fs = 0x10 | 0x3;
    tss_entry.gs = 0x10 | 0x3;
    return entry;
}
