#include <stdint.h>

uint32_t page_directory[1024] __attribute__((aligned(4096)));

void set_up_paging() {
    // set each entry to not be present
    for (int32_t i = 0; i < 1024; i++) {
        // This sets the following flags to the pages:
        //   Supervisor: Only kernel-mode can access them
        //   Write Enabled: It can be both read from and written to
        //   Not Present: The page table is not present
        page_directory[i] = 0x00000002;
    }

    uint32_t first_page_table[1024] __attribute__((aligned(4096)));

    for (uint32_t i = 0; i < 1024; i++) {
        first_page_table[i] = (i * 0x1000) | 3; // attributes: supervisor level, read/write, present.
    }

    page_directory[0] = ((uint32_t) first_page_table) | 3;
    return;
}