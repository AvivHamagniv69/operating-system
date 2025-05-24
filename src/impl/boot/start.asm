MBOOT_PAGE_ALIGN EQU 1 << 0
MBOOT_MEM_INFO EQU 1 << 1
MBOOT_USE_GFX EQU 0
MBOOT_MAGIC EQU 0x1BADB002
MBOOT_FLAGS EQU MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO | MBOOT_USE_GFX
MBOOT_CHECKSUM EQU -(MBOOT_MAGIC + MBOOT_FLAGS)

section .multiboot
ALIGN 4
    DD MBOOT_MAGIC
    DD MBOOT_FLAGS
    DD MBOOT_CHECKSUM

SECTION .bss
ALIGN 16
stack_bottom:
    RESB 16384
stack_top:

section .boot
global _start
_start:
    MOV ecx, (initial_page_dir - 0xC0000000)
    MOV cr3, ecx
    MOV ecx, cr4
    OR ecx, 0x10
    MOV cr4, ecx
    MOV ecx, cr0
    OR ecx, 0x80000001
    MOV cr0, ecx
    JMP higher_half

section .text
higher_half:
    MOV esp, stack_top
    PUSH ebx
    PUSH eax
    XOR ebp, ebp
    extern kmain
    CALL kmain
halt:
    hlt
    JMP halt

section .data
align 4096
global initial_page_dir
initial_page_dir:
    resb 4096

align 4096
table0:
    resb 4096