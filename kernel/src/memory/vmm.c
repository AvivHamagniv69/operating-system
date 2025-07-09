#include "limine.h"
#include <stddef.h>
#include <stdbool.h>
#include "util.h"
#include "serial.h"
#include "pmm.h"
#include "memory_util.h"

// crahses the system, for now replacing with a constant
/*__attribute__((used, section(".limine_requests")))
static volatile struct limine_entry_point_request entry_point_request = {
	.id = LIMINE_ENTRY_POINT_REQUEST,
	.revision = 3
};*/

typedef struct {
	uint64_t pml4;
	uint64_t pml3;
	uint64_t pml2;
	uint64_t pml1;
} PagingIndex;

// todo: add checksum
typedef struct MemTracker {
	bool free;
	void* addr;
	uint64_t len;
	struct MemTracker* next;
} __attribute__((packed)) MemTracker;

MemTracker* mem_tracker_start = NULL;

#define FLAG_PRESENT    (1ULL << 0ULL)
#define FLAG_WRITE      (1ULL << 1ULL)
#define FLAG_READ       (0ULL << 1ULL)
#define FLAG_SUPERVISOR (0ULL << 2ULL)
#define FLAG_USER       (1ULL << 2ULL)
#define PAGING_WRITETHROUGH (1 << 3)
#define PAGING_PHYS_MASK (0x00FFFFFFF000ULL)

#define PML4_AMT 512
#define PML3_AMT 512
#define PML2_AMT 512
#define PML1_AMT 512

static uint64_t* pml4;

static inline void load_cr3(uint64_t cr3_value) {
	__asm__ volatile("mov %0, %%cr3" :: "r"(cr3_value) : "memory");
}

static inline uint64_t get_cr3(void) {
	uint64_t cr3;
	__asm__ volatile ("mov %%cr3, %0" : "=r"(cr3));
	return cr3;
}

static uint64_t HHDM_OFFSET;
static uint64_t KERNEL_VIRT_BASE;

static inline PagingIndex compute_indices(uint64_t vaddr) {
	PagingIndex p = {
		.pml4 = (vaddr >> 39) & 0x1FF,
		.pml3 = (vaddr >> 30) & 0x1FF,
		.pml2 = (vaddr >> 21) & 0x1FF,
		.pml1 = (vaddr >> 12) & 0x1FF,
	};
	return p;
}

static inline uint64_t compute_vaddr(PagingIndex p, uint64_t offset) {
	return ((p.pml4 << 39) | (p.pml3 << 30) | (p.pml2 << 21) | (p.pml1 << 12) | (offset & 0xFFF)); // todo
}

// returns true if theres no index overflow
static inline bool incr_paging_index(PagingIndex p) {
	p.pml1++;
	if(p.pml1 >= PML1_AMT) {
		p.pml1 = 0;
		p.pml2++;
	}
	if(p.pml2 >= PML2_AMT) {
		p.pml2 = 0;
		p.pml3++;
	}
	if(p.pml3 >= PML3_AMT) {
		p.pml3 = 0;
		p.pml4++;
	}
	if(p.pml4 >= PML4_AMT) {
		// reached the final page
		return false;
	}
	return true;
}

static inline uint64_t* access_table(uint64_t* head_table, uint64_t index) {
	return (uint64_t*) ((head_table[index] & PAGING_PHYS_MASK) + HHDM_OFFSET);
}

static uint64_t get_phys_addr(uint64_t vaddr) {
	PagingIndex p = compute_indices(vaddr);
	
	if((pml4[p.pml4] & FLAG_PRESENT) == 0) {
		return 0;
	}
	
	uint64_t* pml3 = access_table(pml4, p.pml4);

	if((pml3[p.pml3] & FLAG_PRESENT) == 0) {
		return 0;
	}
	
	uint64_t* pml2 = access_table(pml3, p.pml3);

	if((pml2[p.pml2] & FLAG_PRESENT) == 0) {
		return 0;
	}

	uint64_t* pml1 = access_table(pml2, p.pml2);

	if((pml1[p.pml1] & FLAG_PRESENT) == 1) {
		// todo: page is already occupied
	}
	return pml1[p.pml1] & PAGING_PHYS_MASK;
}

// function should only be called for a page aligned vaddr, aligns the page anyways
static void map_page(uint64_t vaddr, uint64_t phys_addr, uint64_t flags) {
	PagingIndex p = compute_indices(vaddr);
	
	if((pml4[p.pml4] & FLAG_PRESENT) == 0) {
		uint64_t page = pmm_alloc();
		pml4[p.pml4] = page | FLAG_SUPERVISOR | FLAG_WRITE | FLAG_PRESENT;
		uint64_t* pml3 = access_table(pml4, p.pml4);
		memset(pml3, 0, PAGE_SIZE);
	}
	
	uint64_t* pml3 = access_table(pml4, p.pml4);

	if((pml3[p.pml3] & FLAG_PRESENT) == 0) {
		uint64_t page = pmm_alloc();
		pml3[p.pml3] = page | FLAG_SUPERVISOR | FLAG_WRITE | FLAG_PRESENT;
		uint64_t* pml2 = access_table(pml3, p.pml3);
		memset(pml2, 0, PAGE_SIZE);
	}
	
	uint64_t* pml2 = access_table(pml3, p.pml3);

	if((pml2[p.pml2] & FLAG_PRESENT) == 0) {
		uint64_t page = pmm_alloc();
		pml2[p.pml2] = page | FLAG_SUPERVISOR | FLAG_WRITE | FLAG_PRESENT;
		uint64_t* pml1 = access_table(pml2, p.pml2);
		memset(pml1, 0, PAGE_SIZE);
	}

	uint64_t* pml1 = access_table(pml2, p.pml2);

	if((pml1[p.pml1] & FLAG_PRESENT) == 1) {
		// todo: page is already occupied
	}
	pml1[p.pml1] = phys_addr | FLAG_PRESENT | flags;
	__asm__ volatile("invlpg (%0)" : : "r"(vaddr) : "memory");
}

/*
	todo:
	free an entire pml level when its empty
*/
static void unmap_page(uint64_t vaddr) {
	PagingIndex p = compute_indices(vaddr);
	
	if((pml4[p.pml4] & FLAG_PRESENT) == 0) {
		return;
	}

	uint64_t* pml3 = access_table(pml4, p.pml4);
	if((pml3[p.pml3] & FLAG_PRESENT) == 0) {
		return;
	}
	
	uint64_t* pml2 = access_table(pml3, p.pml3);
	if((pml2[p.pml2] & FLAG_PRESENT) == 0) {
		return;
	}

	uint64_t* pml1 = access_table(pml2, p.pml2);
	if((pml1[p.pml1] & FLAG_PRESENT) == 0) {
		return;
	}
	pmm_free(pml1[p.pml1] & ~0xfff);
	pml1[p.pml1] = 0x0;
	__asm__ volatile("invlpg (%0)" : : "r"(vaddr) : "memory");
}

static uint64_t find_free_vpages(uint64_t pages_amt) {
	uint64_t start = 0x1000;
	uint64_t pages_found = 0;
	for(uint64_t p4_i = 0; p4_i < PML4_AMT; p4_i++) {
		if((pml4[p4_i] & FLAG_PRESENT) == 0) {
			pages_found += PAGE_SIZE * PML1_AMT * PML2_AMT * PML3_AMT;
			if(pages_found >= pages_amt) {
				return start;
			}
			else {
				continue;
			}
		}
		uint64_t* pml3 = access_table(pml4, p4_i);

		for(uint64_t p3_i = 0; p3_i < PML3_AMT; p3_i++) {
			if((pml3[p3_i] & FLAG_PRESENT) == 0) {
				pages_found += PAGE_SIZE * PML1_AMT * PML2_AMT;
				if(pages_found >= pages_amt) {
					return start;
				}
				else {
					continue;
				}
			}
			uint64_t* pml2 = access_table(pml3, p3_i);

			for(uint64_t p2_i = 0; p2_i < PML2_AMT; p2_i++) {
				if((pml2[p2_i] & FLAG_PRESENT) == 0) {
					pages_found += PAGE_SIZE * PML1_AMT;
					if(pages_found >= pages_amt) {
						return start;
					}
					else {
						continue;
					}
				}
				uint64_t* pml1 = access_table(pml2, p2_i);

				for(uint64_t p1_i = 0; p1_i < PML1_AMT; p1_i++) {
					if(p4_i == 0 && p3_i == 0 && p2_i == 0 && p1_i == 0) {
						continue;
					}

					if((pml1[p1_i] & FLAG_PRESENT) == 0) {
						pages_found += PAGE_SIZE;
						if(pages_found >= pages_amt) {
							return start;
						}
					}
					else {
						PagingIndex p = {p4_i, p3_i, p2_i, p1_i};
						start = compute_vaddr(p, 0) + PAGE_SIZE;
						pages_found = 0;
					}
				}
			}
		}
	}
	return 0;
}

static uint64_t heap_start;
static uint64_t heap_end;
static uint64_t heap_ptr;
static uint64_t alloc_num;

// todo, check if system paging mode is correct
void paging_init(void) {
	struct limine_paging_mode_response* paging_response = paging_request.response;
	if(paging_response == NULL) {
		serial_log("paging response is null");
	}
	else if(paging_response->mode != LIMINE_PAGING_MODE_X86_64_4LVL) {
		serial_log("wrong paging mode was activated");
		hcf();
	}

	struct limine_hhdm_response* hhdm_response = hhdm_request.response;
	if(hhdm_response == NULL) {
		serial_log("hhdm response is null");
		hcf();
	}
	HHDM_OFFSET = hhdm_response->offset;

	struct limine_executable_address_response* exe_addr_response = exe_addr_request.response;
	if(exe_addr_response == NULL) {
		serial_log("executable address response is NULL");
		hcf();
	}
	KERNEL_VIRT_BASE = exe_addr_response->virtual_base;

	pml4 = (uint64_t*) (pmm_alloc() + HHDM_OFFSET);
	memset(pml4, 0, PAGE_SIZE);

	struct limine_memmap_response* response = memmap_request.response; 
	if(response == NULL) {
		// todo
		hcf();
	}
	struct limine_memmap_entry** entries = response->entries;
	uint64_t entries_count = response->entry_count;

	extern uint64_t _kernel_virt_end;

	uint64_t kernel_phys_start = exe_addr_response->physical_base;
	uint64_t kernel_virt_start = exe_addr_response->virtual_base;
	uint64_t kernel_virt_end = (uint64_t) &_kernel_virt_end;
	uint64_t kernel_size = kernel_virt_end - kernel_virt_start;

	for(uint64_t i = 0; i < kernel_size; i += PAGE_SIZE) {
		map_page(kernel_virt_start + i, kernel_phys_start + i, FLAG_WRITE | FLAG_PRESENT);
	}

	for(uint64_t i = 0; i < entries_count; i++) {
		struct limine_memmap_entry* entry = entries[i];
		if(entry->type == LIMINE_MEMMAP_BAD_MEMORY ||
		   entry->type == LIMINE_MEMMAP_RESERVED ||
		   entry->type == LIMINE_MEMMAP_ACPI_NVS) {
			continue;
		}

		uint64_t flags = FLAG_WRITE | FLAG_SUPERVISOR;
		if(entry->type == LIMINE_MEMMAP_FRAMEBUFFER) {
			flags |= PAGING_WRITETHROUGH;
		}

		uint64_t start = entry->base / PAGE_SIZE * PAGE_SIZE;
		uint64_t end = ALIGN(entry->base + entry->length);
		uint64_t len = end - start;
		for(uint64_t j = start; j < end; j += PAGE_SIZE) {
			//serial_log("j: ");
			//serial_log_num_unsigned(j);
			//serial_log("\n");
			uint64_t virt = j + HHDM_OFFSET;
			map_page(virt, j, flags);
		}
	}
	map_page(0, 0, 0);
	serial_log("mapping done!\n");

	uint64_t phys_pml4 = (uint64_t)pml4-HHDM_OFFSET;
	//pml4[510] = phys_pml4 | FLAG_PRESENT | FLAG_WRITE | FLAG_SUPERVISOR;

	__asm__ volatile("mov %0, %%cr3" : : "r"(phys_pml4) : "memory");

	heap_start = 0x1000;
	heap_end = heap_start;
	heap_ptr = heap_start;
	alloc_num = 0;
}

void* kmalloc(uint64_t size) {
	if(size == 0) {
		return NULL;
	}

	MemTracker* h = mem_tracker_start;
	while(h != NULL) {
		if(h->free == false || h->len < size) {
			h = h->next;
			continue;
		}

		void* return_addr = h->addr;
		h->free = false;

		uint64_t new_len = h->len - size;
		if(new_len > sizeof(MemTracker)) {
			MemTracker* new_block = (MemTracker*) ((uint64_t)h->addr + size);
			new_block->free = true;
			new_block->len = new_len - sizeof(MemTracker);
			new_block->next = h->next;
			new_block->addr = (void*) (new_block+1);
			h->next = new_block;
		}

		h->len = size;
		return return_addr;
	}

	uint64_t total_size = sizeof(MemTracker) + size;
	bool leftover_memory = false;
	if(total_size % PAGE_SIZE != 0) {
		total_size += sizeof(MemTracker);
		leftover_memory = true;
	}
	uint64_t size_aligned = ALIGN(total_size);
	uint64_t pages_to_alloc = size_aligned / PAGE_SIZE;
	
	uint64_t start = find_free_vpages(pages_to_alloc);
	if(start == 0) {
		return NULL;
	}

	uint64_t p = start;
	for(uint64_t i = 0; i < pages_to_alloc; i++) {
		uint64_t page = pmm_alloc();
		if(page == 0) {
			// todo
			hcf();
		}

		// todo: option for user memory
		map_page(p, page, FLAG_SUPERVISOR | FLAG_WRITE);
		p += PAGE_SIZE;
	}
	
	MemTracker* return_block = (MemTracker*) start;
	return_block->free = false;
	return_block->addr = (void*) (return_block+1);
	return_block->len = size;
	return_block->next = NULL;

	if(mem_tracker_start == NULL) {
		mem_tracker_start = return_block;
	}
	else {
		MemTracker* h = mem_tracker_start;
		while(h->next != NULL) {
			h = h->next;
		}
		h->next = return_block;
	}

	if(leftover_memory) {
		MemTracker* leftover = (MemTracker*) (start + sizeof(MemTracker) + size);
		leftover->free = true;
		leftover->addr = (void*) (leftover+1);
		leftover->len = size_aligned - total_size;
		leftover->next = NULL;
		return_block->next = leftover;
	}

	return return_block->addr;
}

static void combine_free_blocks(MemTracker* start) {
	if(start == NULL || start->free == false) {
		return;
	}
	
	uint64_t total_size = start->len;
	MemTracker* h = start->next;
	while(h != NULL && h->free == true) {
		total_size += h->len + sizeof(MemTracker);
		h = h->next;
	}

	start->len = total_size;
	start->next = h;
}

void kfree(void* addr) {
	MemTracker* tracker = ((MemTracker*) addr)-1;
	if(tracker->addr != addr) {
		// todo
		hcf();
	}
	tracker->free = true;
	combine_free_blocks(tracker);

	MemTracker* prev = mem_tracker_start;
	if(prev != tracker) {
		while(prev->next != tracker) {
			prev = prev->next;
		}	
	}

	MemTracker* next = tracker->next;

	const uint64_t total_size = sizeof(MemTracker) + tracker->len;
	if((uint64_t)tracker % PAGE_SIZE != 0 || total_size < PAGE_SIZE) { // todo: check if its bigger then two pages yet not aligned if so handle that case
		return;
	}
	const uint64_t floor_aligned_size = total_size / PAGE_SIZE * PAGE_SIZE;
	const uint64_t pages_to_free = floor_aligned_size / PAGE_SIZE;	
	uint64_t leftover_addr = (uint64_t)tracker;
	for(uint64_t i = 0; i < pages_to_free; i++, leftover_addr += PAGE_SIZE) {
		uint64_t phys = get_phys_addr(leftover_addr);
		pmm_free(phys);
		unmap_page(leftover_addr);
	}

	const uint64_t leftover_size = total_size - floor_aligned_size;
	if(leftover_size > sizeof(MemTracker)) {
		MemTracker* temp = (MemTracker*) leftover_addr;
		temp->addr = (void*) (temp+1);
		temp->free = true;
		temp->len = leftover_size;
		temp->next = next;
		next = temp;
	}
	// if the left over size is smaller then the size of MemTracker we have no choice but to waste it and leave fragmentation
	if(mem_tracker_start == prev) {
		mem_tracker_start = next;
	}
	else {
		prev->next = next;
	}
}