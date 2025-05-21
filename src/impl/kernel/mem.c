#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "multiboot.h"
#include "util.h"
#include "mem.h"
#include "print.h"

// only took me a week to get this fuckass memory working

#define NUM_PAGES_DIRS 256
#define NUM_PAGE_TABLES 1024
#define NUM_PAGES 1024
#define NUM_PAGE_FRAMES (0x100000000 / 0x1000 / 32)
#define PAGE_FRAME_SIZE 0x1000

#define KERNEL_START 0xC0000000
#define KERNEL_MALLOC 0xD000000
#define REC_PAGEDIR ((uint32_t*)0xFFFFF000)
#define PT_VIRT_ADDR(i) ((uint32_t*)(0xFFC00000 + ((i) << 12)))

#define PAGE_FLAG_PRESENT (1 << 0)
#define PAGE_FLAG_WRITE (1 << 1)
#define PAGE_FLAG_OWNER (1 << 9)

typedef struct BitMapPtr {
    uint32_t index;
    uint32_t bit;
} BitMapPtr;

static uint32_t phys_mem_bitmap[NUM_PAGE_FRAMES]; //Dynamically, bit array

/*static inline void inc_ptr() {
    ptr.bit++;
    if(ptr.bit >= 32) {
        ptr.bit = 0;
        ptr.index++;
    }
    // todo
}*/

static uint32_t page_dirs[NUM_PAGES_DIRS][NUM_PAGE_TABLES] __attribute__((aligned(PAGE_FRAME_SIZE)));
static uint8_t page_dir_used[NUM_PAGES_DIRS];

extern uint32_t initial_page_dir[NUM_PAGE_TABLES];

static uint32_t page_frame_min;
static uint32_t page_frame_max;
static uint32_t vheap_start;
static uint32_t vheap_end;
static uint32_t vheap_ptr;
static uint32_t alloc_num;

static inline void invalidate(uint32_t vaddr){
    __asm__ volatile("invlpg %0" :: "m"(vaddr));
}

static inline uint32_t* get_pt(uint32_t* pd, uint32_t index) {
    return ((uint32_t *)pd) + (0x400 * index);
}

/*static uint32_t get_physaddr(void *virtualaddr) {
    uint32_t pdindex = (uint32_t)virtualaddr >> 22;
    uint32_t ptindex = (uint32_t)virtualaddr >> 12 & 0x03FF;

    uint32_t* pd = (uint32_t *)0xFFFFF000;
    // Here you need to check whether the PD entry is present.
    
    uint32_t* pt = pd + (0x400 * pdindex);
    // Here you need to check whether the PT entry is present.

    return ((pt[ptindex] & ~0xFFF) + ((uint32_t)virtualaddr & 0xFFF));
}*/

static inline uint32_t compute_virtual_address(uint32_t pdi, uint32_t pti, uint32_t offset) {
    return ((pdi << 22) | (pti << 12) | (offset & 0xFFF));
}

// moves the cr3 pointer to a new page directory
static inline void mem_change_page_dir(uint32_t* pd){
    pd = (uint32_t*) (((uint32_t)pd)-KERNEL_START);
    __asm__ volatile("mov %0, %%eax \n mov %%eax, %%cr3 \n" :: "m"(pd));
}

// retrieves the current page directory
static inline uint32_t* mem_get_cur_page_dir(){
    uint32_t pd;
    __asm__ volatile("mov %%cr3, %0": "=r"(pd));
    pd += KERNEL_START;

    return (uint32_t*) pd;
}

// we want the kernel to be mapped in every page dir in 0xC0000000
static void sync_page_dirs(){
    for (int i = 0; i < NUM_PAGES_DIRS; i++){
        if (page_dir_used[i]){
            uint32_t* pageDir = page_dirs[i];

            for (int i = 768; i < 1023; i++){
                pageDir[i] = initial_page_dir[i] & ~PAGE_FLAG_OWNER;
            }
        }
    }
}

// returns a physical address
static uint32_t pmm_alloc_page_frame() {
    uint32_t start = page_frame_min / 32;
    uint32_t end = page_frame_max / 32;

    for (uint32_t i = start; i < end; i++) {
        uint32_t byte = phys_mem_bitmap[i];
        if (byte == 0xFFFFFFFF){
            continue;
        }

        for (uint32_t bit = 0; bit < 32; bit++){
            bool used = (byte >> bit) & 1;

            if (!used){
                byte |= (1 << bit);
                phys_mem_bitmap[i] = byte;
                alloc_num++;

                uint32_t addr = ((i * 32 + bit) * PAGE_FRAME_SIZE);
                return addr;
            }
        }
    }
    return 0;
}

static void pmm_free_page_frame(uint32_t phys_addr) {
    uint32_t frame_num = phys_addr / PAGE_FRAME_SIZE;
    uint32_t index = frame_num / 32;
    uint32_t bit = frame_num % 32;
    uint32_t t = phys_mem_bitmap[index];
    t = t & (0 << bit);
    phys_mem_bitmap[index] = t;
}

void print_cr0_cr3() {
    uint32_t cr0, cr3;

    __asm__ volatile ("mov %%cr0, %0" : "=r"(cr0));
    __asm__ volatile ("mov %%cr3, %0" : "=r"(cr3));

    kprint("CR0 = ");
    kprint_num_u32(cr0);
    kprint("\n");
    kprint("CR3 = ");
    kprint_num_u32(cr3);
    kprint("\n");
}

static void mmap(uint32_t vaddr, uint32_t phys_addr, uint32_t flags) {
    uint32_t* prevPageDir = 0;

    // if were mapping a page in kernel space, we want to do it in the first pd and then sync them together
    if (vaddr >= KERNEL_START){
        prevPageDir = REC_PAGEDIR;
        if (prevPageDir != initial_page_dir){
            mem_change_page_dir(initial_page_dir);
        }
    }

    uint32_t pdIndex = vaddr >> 22;
    uint32_t ptIndex = vaddr >> 12 & 0x3FF;
    
    uint32_t* pageDir = REC_PAGEDIR;
    if((pageDir[pdIndex] & 0x1) == 0) {
        uint32_t p = pmm_alloc_page_frame();
        pageDir[pdIndex] = (p & 0xFFFFF000) | PAGE_FLAG_PRESENT | (PAGE_FLAG_WRITE & 0xFFF);
        __asm__ volatile ("movl	%cr3,%eax\n movl	%eax,%cr3");
    }
    uint32_t* pt = PT_VIRT_ADDR(pdIndex);

    pt[ptIndex] = (phys_addr & 0xFFFFF000) | PAGE_FLAG_PRESENT | (flags & 0xFFF);
    invalidate(vaddr);

    // if we mapped a page in the kernel space we will sync the other pds
    if (prevPageDir != 0){
        sync_page_dirs();

        if (prevPageDir != initial_page_dir){
            mem_change_page_dir(prevPageDir);
        }
    }
}

static void unmap_mmap(uint32_t vaddr) {
    uint32_t* prevPageDir = 0;

    // if were mapping a page in kernel space, we want to do it in the first pd and then sync them together
    if (vaddr >= KERNEL_START){
        prevPageDir = mem_get_cur_page_dir();
        if (prevPageDir != initial_page_dir){
            mem_change_page_dir(initial_page_dir);
        }
    }

    uint32_t pdIndex = vaddr >> 22;
    uint32_t ptIndex = vaddr >> 12 & 0x3FF;
    
    uint32_t* pageDir = REC_PAGEDIR;
    uint32_t* pt = PT_VIRT_ADDR(pdIndex);

    pt[ptIndex] = 0;
    invalidate(vaddr);

    // if we mapped a page in the kernel space we will sync the other pds
    if (prevPageDir != 0){
        sync_page_dirs();

        if (prevPageDir != initial_page_dir){
            mem_change_page_dir(prevPageDir);
        }
    }
}

static uint32_t find_free_vpages(uint32_t size) {
    size = ceil_div(size, PAGE_FRAME_SIZE);
    uint32_t* pd = REC_PAGEDIR;
    uint32_t size_found = 0;
    uint32_t i = 1;
    uint32_t i_start = 1;
    uint32_t j_start = 0;
    bool start_over = false;
    while(i < NUM_PAGE_TABLES && size_found < size) {
        uint32_t* pt = PT_VIRT_ADDR(i);
        if(!(pd[i] & 0x1)) {
            size_found += NUM_PAGE_TABLES * PAGE_FRAME_SIZE;
            continue;
        }
        uint32_t j = 0;
        while(j < NUM_PAGES && size_found < size) {
            if(start_over) {
                i_start = i;
                j_start = j;
                size_found = 0;
                start_over = false;
            }
            if((pt[j] & PAGE_FLAG_PRESENT) == 0) {
                size_found += PAGE_FRAME_SIZE;
            } 
            else { 
                start_over = true;
            }
            j++;
        }
        i++;
    }

    if(size_found < size) {
        return 0;
    }
    return compute_virtual_address(i_start, j_start, 0);
}

static void pmm_init(uint32_t memLow, uint32_t memHigh){
    page_frame_min = ceil_div(memLow, PAGE_FRAME_SIZE);
    page_frame_max = memHigh / PAGE_FRAME_SIZE;

    memset(phys_mem_bitmap, 0, sizeof(phys_mem_bitmap));
}

void init_mem(uint32_t physicalAllocStart, uint32_t memHigh){
    initial_page_dir[0] = 0;
    invalidate(0);
    initial_page_dir[1023] = ((uint32_t) initial_page_dir - KERNEL_START) | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE;
    invalidate(0xFFFFF000);

    mem_change_page_dir(initial_page_dir);
    
    pmm_init(physicalAllocStart, memHigh);
    memset(page_dirs, 0, PAGE_FRAME_SIZE * NUM_PAGES_DIRS);
    memset(page_dir_used, 0, NUM_PAGES_DIRS);

    vheap_start = compute_virtual_address(1, 0, 0);
    vheap_end = vheap_start;
    vheap_ptr = vheap_start;
    alloc_num = 0;
}

// using a bump allocator
// if no space:
// mark a new page as used
// find a new page that starts from vheap_end
// find a new physical page
// alloc_num++
// map them
// if space:
// calc the vaddr
// add the size to ptr
// alloc_num--

//uint32_t pdindex = (uint32_t)virtualaddr >> 22;
//uint32_t ptindex = (uint32_t)virtualaddr >> 12 & 0x03FF;
// offfset = vaddr & 0xfff

void* kmalloc(uint32_t size) {
    // free mem available
    if(vheap_end - vheap_ptr > size) {
        void* vaddr = (void*)vheap_ptr;
        vheap_ptr += size;
        alloc_num++;
        return vaddr;
    }
    // no free mem available
    uint32_t page_to_alloc = ceil_div(size, PAGE_FRAME_SIZE);
    uint32_t vaddr = find_free_vpages(page_to_alloc * PAGE_FRAME_SIZE);
    uint32_t pdi = vaddr >> 22;
    uint32_t pti = vaddr >> 12 & 0x03FF;
    for(uint32_t i = 0; i < page_to_alloc; i++) {
        uint32_t phys_addr = pmm_alloc_page_frame();
        mmap(compute_virtual_address(pdi, pti, 0), phys_addr, PAGE_FLAG_OWNER | PAGE_FLAG_WRITE); // todo
        // inc_ptr();
        pti++;
        if(pti >= NUM_PAGE_FRAMES) {
            pti = 0;
            pdi++;
        }
    }
    vheap_end = compute_virtual_address(pdi, pti, 0);
    vheap_ptr += size;
    alloc_num++;
    return (void*)vaddr;
}