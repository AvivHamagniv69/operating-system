#include <stdint.h>

typedef struct {
    uint32_t base;
    uint32_t limit;
    uint8_t access_byte;
    uint8_t flags;
    uint16_t offset;
} __attribute__((packed)) GDTEntry;

void encode_gdt_entry(uint8_t *target, GDTEntry source) {
    if (source.limit > 0xFFFFF) {
        // TODO
    }

    // encode limit
    target[0] = source.limit & 0xFF;
    target[1] = (source.limit >> 8) & 0xFF;
    target[6] = (source.limit >> 16) & 0x0F;

    // encode base
    target[2] = source.base & 0xFF;
    target[3] = (source.base >> 8) & 0xFF;
    target[4] = (source.base >> 16) & 0xFF;
    target[7] = (source.base >> 24) & 0xFF;
    
    // encode access byte
    target[5] = source.access_byte;

    // encode flags
    target[6] |= (source.flags << 4);
}