#include <stdint.h>
#include <stddef.h>
//#include "print.h"

extern void load_page_directory(uint32_t* page_directory);
extern void enable_paging();
extern void disable_paging();
extern void enable_PAE();

void kmain() {
    uint32_t page_directory[1024] __attribute__((aligned(4096)));

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

    load_page_directory(page_directory);
    enable_paging();
    disable_paging();

    uint64_t page_dir_ptr_tab[4] __attribute__((aligned(0x20))); // must be aligned to (at least)0x20, ...
    // ... turning out that you can put more of them into one page, saving memory

    // 512 entries
    uint64_t page_dir[512] __attribute__((aligned(0x1000)));  // must be aligned to page boundary
    
    page_dir_ptr_tab[0] = (uint64_t)&page_dir | 1; // set the page directory into the PDPT and mark it present
    page_dir[0] = 0b10000011; //Address=0, 2MIB, RW and present

    __asm__ __volatile__ ("mov %%cr4, %%eax; bts $5, %%eax; mov %%eax, %%cr4;" ::: "eax"); // set bit5 in CR4 to enable PAE		 
    __asm__ __volatile__ ("mov %0, %%cr3;" :: "r" (&page_dir_ptr_tab)); // load PDPT into CR3
    __asm__ __volatile__ ("mov %%cr0, %%eax; orl $0x80000000, %%eax; mov %%eax, %%cr0;" ::: "eax");

    //clear_screen();
    //print_str("hello world", 11);
    return;
}