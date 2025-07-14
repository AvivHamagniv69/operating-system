#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "util.h"
#include "memory/pmm.h"
#include "memory/vmm.h"
#include "idt.h"
#include "serial.h"
#include "pit.h"
#include "ps2.h"
#include "print/print.h"

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
    pit_init();
    init_keyboard();
    //__asm__ volatile ("div %0" :: "r"(0));
    pmm_init();
    paging_init();
    serial_log("paging done!\n");

    unsigned int* a = kmalloc(sizeof(unsigned int));
    *a = 69;
    serial_log_num_unsigned(*a);

    int* b = (int*) kmalloc(sizeof(int));

    kfree(b);
    kfree(a);

    // We're done, just hang...
    hcf();
}
