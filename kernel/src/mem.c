#include "limine.h"
#include <stddef.h>
#include <stdbool.h>
#include "mem.h"
#include "util.h"
#include "serial.h"

#define LIMINE_PAGING_MODE_X86_64_4LVL 0

#define LIMINE_PAGING_MODE_DEFAULT LIMINE_PAGING_MODE_X86_64_4LVL
#define LIMINE_PAGING_MODE_MIN LIMINE_PAGING_MODE_X86_64_4LVL
#define LIMINE_PAGING_MODE_MAX LIMINE_PAGING_MODE_X86_64_4LVL

__attribute__((used, section(".limine_requests")))
static volatile struct limine_paging_mode_request paging_request = {
    .id = LIMINE_PAGING_MODE_REQUEST,
    .revision = 0,
    .mode = LIMINE_PAGING_MODE_DEFAULT,
    .max_mode = LIMINE_PAGING_MODE_MAX,
    .min_mode = LIMINE_PAGING_MODE_MIN,
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

// crahses the system, for now replacing with a constant
/*__attribute__((used, section(".limine_requests")))
static volatile struct limine_entry_point_request entry_point_request = {
    .id = LIMINE_ENTRY_POINT_REQUEST,
    .revision = 0
};*/

typedef struct {
    uint64_t pml4;
    uint64_t pml3;
    uint64_t pml2;
    uint64_t pml1;
} PagingIndex;

#define FLAG_PRESENT    (1 << 0)
#define FLAG_WRITE      (1 << 1)
#define FLAG_READ       (0 << 1)
#define FLAG_SUPERVISOR (0 << 2)
#define FLAG_USER       (1 << 2)

#define PAGE_SIZE 4096
#define ALIGN(l) ((((l) + PAGE_SIZE - 1)/PAGE_SIZE)*PAGE_SIZE)

#define PML4_AMT 512
#define PML3_AMT 512
#define PML2_AMT 512
#define PML1_AMT 512

static uint64_t pml4[PML4_AMT] __attribute__((aligned(PAGE_SIZE)));;

static uint64_t pmm_len;
static uint64_t* phys_mem_bitmap __attribute__((aligned(PAGE_SIZE)));;

typedef struct {
    uint64_t index;
    uint64_t bit;
} PmmPtr;

static inline void flush_tlb(void* page) 
{
	__asm__ volatile ("invlpg (%0)" :: "r" (page) : "memory");
}

static inline void load_cr3(void* cr3_value) {
    __asm__ volatile("mov %0, %%cr3" :: "r"((uint64_t)cr3_value) : "memory");
}

static inline uint64_t* get_cr3(void) {
    uint64_t cr3;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(cr3));
    return (uint64_t*)cr3;
}

// returns 1 if bit is not free, zero if free
static inline bool is_bit_free(uint64_t index, uint64_t bit) {
    uint64_t byte = phys_mem_bitmap[index];
    return byte & (1 << bit);
}

static inline PmmPtr addr_to_pmm_indexing(uint64_t addr) {
    // todo: check if correct
    PmmPtr p;
    p.index = addr / PAGE_SIZE / 64;
    p.bit = addr / PAGE_SIZE;
    return p;
}

static inline uint64_t pmm_bit_to_addr(uint64_t index, uint64_t bit) {
    // todo: check if correct
    uint64_t addr = (index * 64 + bit) * PAGE_SIZE;
    return addr;
}

static inline void mark_pmm_bit_taken(uint64_t index, uint64_t bit) {
    uint64_t byte = phys_mem_bitmap[index];
    byte = byte | (1 << bit);
    phys_mem_bitmap[index] = byte;
}

static inline void mark_pmm_bit_free(uint64_t index, uint64_t bit) {
    uint64_t byte = phys_mem_bitmap[index];
    byte = byte & (0 << bit);
    phys_mem_bitmap[index] = byte;
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

static struct limine_memmap_entry** entries;
static uint64_t entries_count;
static uint64_t highest_page;
#define HIGHER_HALF_DATA_LV4	0xFFFF800000000000UL
#define HIGHER_HALF_CODE	0xFFFFFFFF80000000UL
#define ENTRY_FREE 0 // todo: idk which number represents a free entry is free

static void pmm_init() {
    struct limine_memmap_response* response = memmap_request.response; 
    if(response == NULL) {
        // todo
        hcf();
    }
    entries = response->entries;
    entries_count = response->entry_count;

    for(uint64_t i = 0; i < entries_count; i++) {
        struct limine_memmap_entry* entry = entries[i];
        if(entry->type != ENTRY_FREE) {
            continue;
        } 

        uint64_t top = entry->base + entry->length;
        if(top > highest_page) {
            highest_page = top;
        }
    }

    pmm_len = ALIGN(highest_page) / PAGE_SIZE / 64;
    for(uint64_t i = 0; i < entries_count; i++) {
        struct limine_memmap_entry* entry = entries[i];
        if(entry->type != ENTRY_FREE) {
            continue;
        }

        if(entry->length >= pmm_len) {
            phys_mem_bitmap = (uint64_t*)(HIGHER_HALF_DATA_LV4 + entry->base);
            break;
        }
    }
    memset(phys_mem_bitmap, 0x0, pmm_len);

    for(uint64_t i = 0; i < entries_count; i++) {
        struct limine_memmap_entry* entry = entries[i];

        if(entry->type == ENTRY_FREE) {
            PmmPtr start = addr_to_pmm_indexing(entry->base);
            PmmPtr end = addr_to_pmm_indexing(entry->base + entry->length);
            
            for(uint64_t j = start.index; j < end.index; j++) {
                uint64_t bit_end;
                if(j+1 == end.index) {
                    bit_end = end.bit;
                }
                else {
                    bit_end = 64;
                }
                for(uint64_t h = 0; h < bit_end; h++) {
                    mark_pmm_bit_free(j, h);
                }
            }
        }
    }

    // reserve null pointer
    mark_pmm_bit_taken(0, 0);
}

static uint64_t pmm_find_first_free_page() {
    PmmPtr p = {.index = 0, .bit = 0};
    uint64_t i = 0;
    uint64_t len = pmm_len * 64;
    while(is_bit_free(p.index, p.bit) && i < len) {
        incr_ptr(&p);
        i++;
    }
    // reached the final page and nothing is free
    if(is_bit_free(p.index, p.bit)) {
        return 0;
    }
    return pmm_bit_to_addr(p.index, p.bit);
}

static uint64_t pmm_alloc() {
    uint64_t page = pmm_find_first_free_page();
    if(!page) {
        return 0;
    }
    PmmPtr p = addr_to_pmm_indexing(page);
    mark_pmm_bit_taken(p.index, p.bit);
    return page;
}

static inline void pmm_free(uint64_t page) {
    PmmPtr p = addr_to_pmm_indexing(page);
    mark_pmm_bit_free(p.index, p.bit);
}

static inline PagingIndex compute_indices(uint64_t vaddr) {
    PagingIndex p = {
        .pml4 = (vaddr >> 39) & 0x1FF,
        .pml3 = (vaddr >> 30) & 0x1FF,
        .pml2 = (vaddr >> 21) & 0x1FF,
        .pml1 = (vaddr >> 12) & 0x1FF,
    };
    return p;
}

static inline uint64_t compute_vaddr(uint64_t pml4, uint64_t pml3, uint64_t pml2, uint64_t pml1, uint64_t offset) {
    return ((pml4 << 39) | (pml3 << 30) | (pml2 << 21) | (pml1 << 12) | (offset & 0xFFF)); // todo
}

// function should only be called for a page aligned vaddr, aligns the page anyways
static void mmap(uint64_t vaddr, uint64_t phys_addr, uint64_t flags) {
    PagingIndex p = compute_indices(vaddr);
    
    if((pml4[p.pml4] & FLAG_PRESENT) == 0) {
        uint64_t page = pmm_alloc();
        pml4[p.pml4] = page | FLAG_SUPERVISOR | FLAG_WRITE | FLAG_PRESENT;
        load_cr3(get_cr3());
    }

    uint64_t* pml3 = (uint64_t*) (pml4[p.pml4] << 12);
    if((pml3[p.pml3] & FLAG_PRESENT) == 0) {
        uint64_t page = pmm_alloc();
        pml3[p.pml3] = page | FLAG_SUPERVISOR | FLAG_WRITE | FLAG_PRESENT;
        load_cr3(get_cr3());
    }
    
    uint64_t* pml2 = (uint64_t*) (pml3[p.pml3] << 12);
    if((pml2[p.pml2] & FLAG_PRESENT) == 0) {
        uint64_t page = pmm_alloc();
        pml2[p.pml2] = page | FLAG_SUPERVISOR | FLAG_WRITE | FLAG_PRESENT;
        load_cr3(get_cr3());
    }

    uint64_t* pml1 = (uint64_t*) (pml2[p.pml2] << 12);
    if((pml4[p.pml4] & FLAG_PRESENT) == 1) {
        // todo: page is already occupied
    }
    pml1[p.pml1] = phys_addr | FLAG_PRESENT | flags;
    flush_tlb((void*)vaddr);
}

// todo: free an entire pml level when its empty
static void free_page(uint64_t vaddr) {
    PagingIndex p = compute_indices(vaddr);
    
    if((pml4[p.pml4] & FLAG_PRESENT) == 0) {
        return;
    }

    uint64_t* pml3 = (uint64_t*) pml4[p.pml4];
    if((pml3[p.pml3] & FLAG_PRESENT) == 0) {
        return;
    }
    
    uint64_t* pml2 = (uint64_t*) pml3[p.pml3];
    if((pml2[p.pml2] & FLAG_PRESENT) == 0) {
        return;
    }

    uint64_t* pml1 = (uint64_t*) pml2[p.pml2];
    if((pml4[p.pml4] & FLAG_PRESENT) == 0) {
        return;
    }
    pmm_free(pml1[p.pml1] << 12);
    pml1[p.pml1] = 0x0;
    flush_tlb((void*)vaddr);
}

static uint64_t find_free_vpages(uint64_t pages_amt) {
    uint64_t pages_found = 0;
    uint64_t start = 0x0;
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
        uint64_t* pml3 = (uint64_t*) pml4[p4_i];

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
            uint64_t* pml2 = (uint64_t*) pml3[p3_i];

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
                uint64_t* pml1 = (uint64_t*) pml2[p2_i];

                for(uint64_t p1_i = 0; p1_i < PML1_AMT; p1_i++) {
                    if((pml1[p1_i] & FLAG_PRESENT) == 0) {
                        pages_found += PAGE_SIZE;
                        if(pages_found >= pages_amt) {
                            return start;
                        }
                    }
                    else {
                        start = compute_vaddr(p4_i, p3_i, p2_i, p1_i, 0) + PAGE_SIZE;
                    }
                }
            }
        }
    }
    return 0;
}

static void mmap_before_init(uint64_t vaddr, uint64_t phys_addr, uint64_t flags) {
    PagingIndex p = compute_indices(vaddr);
    
    if((pml4[p.pml4] & FLAG_PRESENT) == 0) {
        uint64_t page = pmm_alloc();
        pml4[p.pml4] = page | FLAG_SUPERVISOR | FLAG_WRITE | FLAG_PRESENT;
    }

    uint64_t* pml3 = (uint64_t*) (pml4[p.pml4] << 12);
    if((pml3[p.pml3] & FLAG_PRESENT) == 0) {
        uint64_t page = pmm_alloc();
        pml3[p.pml3] = page | FLAG_SUPERVISOR | FLAG_WRITE | FLAG_PRESENT;
    }
    
    uint64_t* pml2 = (uint64_t*) (pml3[p.pml3] << 12);
    if((pml2[p.pml2] & FLAG_PRESENT) == 0) {
        uint64_t page = pmm_alloc();
        pml2[p.pml2] = page | FLAG_SUPERVISOR | FLAG_WRITE | FLAG_PRESENT;
    }

    uint64_t* pml1 = (uint64_t*) (pml2[p.pml2] << 12);
    if((pml4[p.pml4] & FLAG_PRESENT) == 1) {
        // todo: page is already occupied
    }
    pml1[p.pml1] = phys_addr | FLAG_PRESENT | flags;
}

static uint64_t heap_start;
static uint64_t heap_end;
static uint64_t heap_ptr;
static uint64_t alloc_num;

void paging_init(void) {
    // todo, check if system paging mode is correct
    pmm_init();

    uint64_t map1_start = 0xffff800000001000;
    uint64_t map1_end =   0xffff8000000a0000;
    uint64_t phys_1 =     0x000000000009f000;
    
    uint64_t map2_start = 0xffff800000100000;
    uint64_t map2_end =   0xffff80007ffdf000;
    uint64_t phys_2 =     0x000000007fedf000;

    uint64_t map3_start = 0xffff8000fd000000;
    uint64_t map3_end =   0xffff8000fd3e8000;
    uint64_t phys_3 =     0x00000000003e8000;

    uint64_t map4_start = 0xffffffff80000000;
    uint64_t map4_end =   0xffffffff80001000;
    uint64_t phys_4 =     0x0000000000001000;

    uint64_t map5_start = 0xffffffff80001000; // only r data
    uint64_t map5_end =   0xffffffff80004000;
    uint64_t phys_5 =     0x0000000000003000;

    uint64_t map6_start = 0xffffffff80004000;
    uint64_t map6_end =   0xffffffff80009000;
    uint64_t phys_6 =     0x0000000000005000;
    
    uint64_t maps_start[] = {
        map1_start,
        map2_start,
        map3_start,
        map4_start,
        map5_start,
        map6_start,
    };

    uint64_t maps_end[] = {
        map1_end,
        map2_end,
        map3_end,
        map4_end,
        map5_end,
        map6_end,
    };

    uint64_t maps_phys[] = {
        phys_1,
        phys_2,
        phys_3,
        phys_4,
        phys_5,
        phys_6,
    };

    for(uint64_t i = 0; i < 6; i++) {
        uint64_t phys = maps_phys[i];
        for(uint64_t j = maps_start[i]; j < maps_end[i]; j += PAGE_SIZE) {
            if(i == 4) {
                mmap_before_init(j, phys, FLAG_SUPERVISOR | FLAG_READ);
            }
            else {
                mmap_before_init(j, phys, FLAG_SUPERVISOR | FLAG_WRITE);
            }
            phys += PAGE_SIZE;
        }
    }

    load_cr3(pml4);
    heap_start = compute_vaddr(0, 0, 0, 1, 0);
    heap_end = heap_start;
    heap_ptr = heap_start;
    alloc_num = 0;
}

void* kmalloc(uint64_t size) {
    if(size == 0) {
        return NULL;
    }
    
    if(heap_end - heap_ptr >= size) {
        void* addr = (void*) heap_ptr;
        heap_ptr += size;
        alloc_num++;
        return addr;
    }

    // for some ungodly reason gcc decides it would be best to optimize this vairable out
    //uint64_t volatile pages_to_alloc = ceil_div(size, PAGE_SIZE);
    uint64_t volatile pages_to_alloc = 1;
    serial_log("pages to alloc: ");
    serial_log_num_unsigned(pages_to_alloc);
    serial_log("\n");
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
        mmap(p, page, FLAG_SUPERVISOR | FLAG_WRITE);
        p += PAGE_SIZE;
    }
    heap_end += ALIGN(size);
    heap_ptr += size;

    return (void*) start;
}

void kfree(void* addr) {

}