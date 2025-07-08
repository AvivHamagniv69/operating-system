#include "memory_util.h"
#include "../limine.h"

__attribute__((used, section(".limine_requests")))
volatile struct limine_memmap_request memmap_request = {
	.id = LIMINE_MEMMAP_REQUEST,
	.revision = 3
};

__attribute__((used, section(".limine_requests")))
volatile struct limine_paging_mode_request paging_request = {
	.id = LIMINE_PAGING_MODE_REQUEST,
	.revision = 3,
	.mode = LIMINE_PAGING_MODE_DEFAULT,
	.max_mode = LIMINE_PAGING_MODE_MAX,
	.min_mode = LIMINE_PAGING_MODE_MIN,
};

__attribute__((used, section(".limine_requests")))
volatile struct limine_hhdm_request hhdm_request = {
	.id = LIMINE_HHDM_REQUEST,
	.revision = 3
};

__attribute__((used, section(".limine_requests")))
volatile struct limine_executable_address_request exe_addr_request = {
	.id = LIMINE_EXECUTABLE_ADDRESS_REQUEST,
	.revision = 3
};
