#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../limine.h"
#include "pmm.h"
#include "memory_util.h"
#include "../serial.h"
#include "../util.h"

#define LIMINE_MEMMAP_USABLE                 0
#define LIMINE_MEMMAP_RESERVED               1
#define LIMINE_MEMMAP_ACPI_RECLAIMABLE       2
#define LIMINE_MEMMAP_ACPI_NVS               3
#define LIMINE_MEMMAP_BAD_MEMORY             4
#define LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE 5
#define LIMINE_MEMMAP_EXECUTABLE_AND_MODULES 6
#define LIMINE_MEMMAP_FRAMEBUFFER            7

#define PMM_BITS_PER_ELEMENT 64ULL

typedef struct {
	uint64_t index;
	uint64_t bit;
} PmmPtr;

static uint64_t pmm_len;
static uint64_t* phys_mem_bitmap = NULL;
static uint64_t highest_page;
static uint64_t lowest_page;

static PmmPtr pmm_addr_to_index(uint64_t addr) {
	if(addr < lowest_page) {
		serial_log("INTEGER OVERFLOW DETECTED!");
		hcf();
	}

	addr -= lowest_page;
	addr /= PAGE_SIZE;
	PmmPtr p;
	p.index = addr / PMM_BITS_PER_ELEMENT;
	p.bit = addr - p.index * PMM_BITS_PER_ELEMENT;
	return p;
}

static uint64_t pmm_index_to_addr(PmmPtr p) {
	uint64_t addr = p.bit + p.index * PMM_BITS_PER_ELEMENT;
	addr *= PAGE_SIZE;
	addr += lowest_page;
	return addr;
}

static inline uint8_t get_bit(PmmPtr p) {
	uint64_t byte = phys_mem_bitmap[p.index];
	return (byte & (1ULL << p.bit)) >> p.bit;
}

static inline void pmm_set_bit(uint64_t addr) {
	PmmPtr p = pmm_addr_to_index(addr);
	uint64_t byte = phys_mem_bitmap[p.index];
	byte |= (1ULL << p.bit);
	phys_mem_bitmap[p.index] = byte;
}

static inline void pmm_clear_bit(uint64_t addr) {
	PmmPtr p = pmm_addr_to_index(addr);
	uint64_t byte = phys_mem_bitmap[p.index];
	byte |= (0ULL << p.bit);
	phys_mem_bitmap[p.index] = byte;
}

static inline void incr_ptr(PmmPtr* ptr) {
	if(!ptr) {
		return;
	}
	ptr->bit++;
	if(ptr->bit > 63) {
		ptr->index++;
		ptr->bit = 0;
	}
}

static uint64_t pmm_find_first_free_page() {
	PmmPtr p = {.index = 0, .bit = 0};
	uint64_t i = 0;
	uint64_t len = pmm_len * PMM_BITS_PER_ELEMENT;
	while(get_bit(p) == 1 && i < len) {
		incr_ptr(&p);
		i++;
	}
	// reached the final page and nothing is free
	if(get_bit(p) == 1) {
		return 0;
	}
	return pmm_index_to_addr(p);
}

void pmm_free(uint64_t page) {
	pmm_clear_bit(page);
}

uint64_t pmm_alloc() {
	uint64_t page = pmm_find_first_free_page();
	if(!page) {
		serial_log("NO FREE PHYSICAL RAM LEFT!");
		hcf();
		return 0;
	}
	//serial_log("page: ");
	//serial_log_num_unsigned(page);
	//serial_log("\n");
	pmm_set_bit(page);
	return page;
}

// usable and reclaimble memory is guaranteed to be aligned to 4096 bytes for both base and 
// length, thus no alignments are needed to be done by us.
void pmm_init() {
	struct limine_hhdm_response* hhdm_response = hhdm_request.response;
	if(hhdm_response == NULL) {
		serial_log("hhdm response is null");
		hcf();
	}
	uint64_t HHDM_OFFSET = hhdm_response->offset;

	struct limine_memmap_response* response = memmap_request.response; 
	if(response == NULL) {
		// todo
		hcf();
	}
	struct limine_memmap_entry** entries = response->entries;
	uint64_t entries_count = response->entry_count;

	// get the highest and lowest free memory available
	lowest_page = entries[0]->base;
	highest_page = lowest_page;
	for(uint64_t i = 0; i < entries_count; i++) {
		struct limine_memmap_entry* entry = entries[i];		
		if(entry->type != LIMINE_MEMMAP_USABLE) {
			continue;
		}

		uint64_t top = entry->base + entry->length;
		if(top > highest_page) {
			highest_page = top;
		}
	}
	if(lowest_page == highest_page) {
		serial_log("NO FREE MEMORY AVAILABLE!");
		hcf();
	}

	// find memory for pmm
	pmm_len = (highest_page - lowest_page) / PAGE_SIZE / PMM_BITS_PER_ELEMENT;
	for(uint64_t i = 0; i < entries_count; i++) {
		struct limine_memmap_entry* entry = entries[i];		
		if(entry->type != LIMINE_MEMMAP_USABLE || entry->length < pmm_len) {
			continue;
		}
		phys_mem_bitmap = (uint64_t*) (entry->base + HHDM_OFFSET);
		break;
	}
	if(phys_mem_bitmap == NULL) {
		serial_log("NOT ENOUGH FREE MEMORY FOR BOOT PROCESS!");
		hcf();
	}
	memset(phys_mem_bitmap, ~0, pmm_len);

	// mark available memory
	for(uint64_t i = 0; i < entries_count; i++) {
		struct limine_memmap_entry* entry = entries[i];		
		if(entry->type != LIMINE_MEMMAP_USABLE) {
			continue;
		}

		uint64_t start = entry->base;
		uint64_t end = entry->base + entry->length;
		for(uint64_t j = start; j < end; j += PAGE_SIZE) {
			pmm_clear_bit(j);
		}
	}

	// mark unavailble memory used by the pmm
	uint64_t start = (uint64_t)phys_mem_bitmap - HHDM_OFFSET;
	uint64_t end = start + ALIGN(pmm_len);
	for(uint64_t i = start; i < end; i += PAGE_SIZE) {
		pmm_set_bit(i);
	}
}
