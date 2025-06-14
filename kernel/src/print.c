
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "limine.h"
#include "util.h"

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, usually, they should
// be made volatile or equivalent, _and_ they should be accessed at least
// once or marked as used with the "used" attribute as done here.

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 3
};

static struct limine_framebuffer *framebuffer;

static uint32_t* vram; // todo, check framebuffer model
static uint64_t width;
static uint64_t height;
static uint64_t pitch;
static uint16_t bits_per_pixel;

void kprint_init(void) {
    // Ensure we got a framebuffer.
    if (framebuffer_request.response == NULL
     || framebuffer_request.response->framebuffer_count < 1) {
        hcf();
    }

    // Fetch the first framebuffer.
    framebuffer = framebuffer_request.response->framebuffers[0];

    vram = framebuffer->address;
    width = framebuffer->width;
    height = framebuffer->height;
    pitch = framebuffer->pitch;
    bits_per_pixel = framebuffer->bpp;

    // Note: we assume the framebuffer model is RGB with 32-bit pixels.
    /*for (size_t i = 0; i < 100; i++) {
        volatile uint32_t *fb_ptr = framebuffer->address;
        fb_ptr[i * (framebuffer->pitch / 4) + i] = 0xffffff;
    }*/
}

#define GET_PIXEL(x, y) ((uint32_t*)(vram + y*pitch + x*bits_per_pixel))

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

static void pchar(char c) {

}

void kprint(char* str) {
}