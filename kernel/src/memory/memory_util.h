#include "../limine.h"

#define LIMINE_PAGING_MODE_X86_64_4LVL 0
#define LIMINE_PAGING_MODE_DEFAULT LIMINE_PAGING_MODE_X86_64_4LVL
#define LIMINE_PAGING_MODE_MIN LIMINE_PAGING_MODE_X86_64_4LVL
#define LIMINE_PAGING_MODE_MAX LIMINE_PAGING_MODE_X86_64_4LVL

#define ALIGN(l) ((((l) + PAGE_SIZE - 1)/PAGE_SIZE)*PAGE_SIZE)

extern volatile struct limine_memmap_request memmap_request;

extern volatile struct limine_paging_mode_request paging_request;

extern volatile struct limine_hhdm_request hhdm_request;

extern volatile struct limine_executable_address_request exe_addr_request;
