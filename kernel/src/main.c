#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "util.h"
#include "mem.h"
#include "idt.h"
#include "print.h"
#include "serial.h"
#include "pit.h"

__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3);

// Finally, define the start and end markers for the Limine requests.
// These can also be moved anywhere, to any .c file, as seen fit.

__attribute__((used, section(".limine_requests_start")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile LIMINE_REQUESTS_END_MARKER;

// The following will be our kernel's entry point.
// If renaming kmain() to something else, make sure to change the
// linker script accordingly.
void kmain(void) {
    // Ensure the bootloader actually understands our base revision (see spec).
    if (LIMINE_BASE_REVISION_SUPPORTED == false) {
        hcf();
    }
    serial_init();
    kprint_init();
    idt_init();
    // pit_init();
    __asm__ volatile ("div %0" :: "r"(0));
    paging_init();

    int* a = kmalloc(sizeof(int));
    *a = 69;
    
    // We're done, just hang...
    hcf();
}
