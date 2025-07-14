
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "../limine.h"
#include "../util.h"

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, usually, they should
// be made volatile or equivalent, _and_ they should be accessed at least
// once or marked as used with the "used" attribute as done here.

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 3
};

#define PSF1_FONT_MAGIC 0x0436

typedef struct {
    uint16_t magic; // Magic bytes for identification.
    uint8_t font_mode; // PSF font_header mode.
    uint8_t character_size; // PSF character size.
} PSF1_Header;

#define SPACING 1

#define WIDTH_IN_BITS 8
#define HEIGHT_IN_BITS 16
#define BYTES_PER_GLYPH 16

extern char _binary_src_print_ascii_psf_start;
extern char _binary_src_print_ascii_psf_size;

uint64_t HEADER_SIZE = (uint64_t)&_binary_src_print_ascii_psf_size;

PSF1_Header* font_header;

void psf_init() {
    font_header = (PSF1_Header*) &_binary_src_print_ascii_psf_start;
}

static uint8_t* GET_GLYPH(uint8_t i) {
    uint8_t* ret = (uint8_t*) &_binary_src_print_ascii_psf_start;
    ret += sizeof(PSF1_Header);
    ret += i * BYTES_PER_GLYPH;
    return ret;
}

// #define GET_GLYPH(i) (uint8_t*) &_binary_src_print_ascii_psf_size + \
// sizeof(PSF1_Header) + (i * BYTES_PER_GLYPH)

static struct limine_framebuffer *framebuffer;
uint32_t* addr;

void kprint_init(void) {
    // Ensure we got a framebuffer.
    if (framebuffer_request.response == NULL
     || framebuffer_request.response->framebuffer_count < 1) {
        hcf();
    }

    // Fetch the first framebuffer.
    framebuffer = framebuffer_request.response->framebuffers[0];
    addr = framebuffer->address;

    psf_init();

    // Note: we assume the framebuffer model is RGB with 32-bit pixels.
    // for (size_t i = 0; i < 100; i++) {
    //     volatile uint32_t *fb_ptr = framebuffer->address;
    //     fb_ptr[i * (framebuffer->pitch / 4) + i] = 0xffffff;
    // }
}

#define GET_PIXEL(x, y) ((uint32_t*)(addr + (y)*framebuffer->pitch/4 + (x)))

static inline void put_pixel(uint16_t x, uint16_t y) {
    uint32_t* pixel = GET_PIXEL(x, y);
    *pixel = 0xffffff; // todo, custom colors
}

static void fill_rect(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end) {
    for(uint32_t i = y_start; i < y_end; i++) {
        for(uint32_t j = x_start; j < x_end; j++) {
            uint32_t* p = GET_PIXEL(j, i);
            *p = 0xffffff;
        }
    }
}

void pchar(char c, uint16_t x, uint16_t y) {
    uint8_t* glyph = GET_GLYPH((uint8_t) c);
    uint16_t offset_x = x;
    uint16_t offset_y = y;
    for(uint16_t i = 0; i < HEIGHT_IN_BITS; i++) {
        uint8_t byte = glyph[i];
        offset_x = x;
        for(uint16_t j = WIDTH_IN_BITS; j > 0; j--) {
            bool pixel_filled = (byte & (1 << j)) >> j;
            if(pixel_filled) {
                put_pixel(offset_x, offset_y);
            }
            offset_x++;
        }
        offset_y++;  
    }
}

void kprint(char* str) {
    static uint16_t curr_x = 0;
    static uint16_t curr_y = 0;    

    while(*str) {
        pchar(*str, curr_x, curr_y);
        curr_x += WIDTH_IN_BITS + SPACING;
        str++;
    }
}