section .multiboot
    ALIGN 4
    MBOOT_PAGE_ALIGN EQU 1 << 0
    MBOOT_MEM_INFO EQU 1 << 1
    MBOOT_USE_GFX EQU 0

    MBOOT_MAGIC EQU 0x1BADB002
    MBOOT_FLAGS EQU MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO | MBOOT_USE_GFX
    MBOOT_CHECKSUM EQU -(MBOOT_MAGIC + MBOOT_FLAGS)

    DD MBOOT_MAGIC
    DD MBOOT_FLAGS
    DD MBOOT_CHECKSUM
    DD 0, 0, 0, 0, 0

    DD 0
    DD 800
    DD 600
    DD 32

SECTION .bss
ALIGN 16
stack_bottom:
    RESB 16384
stack_top:

section .boot
bits 32
extern _kernel_start
extern _kernel_end
global _start
_start:
    mov esi, 0x0
    mov edi, 0x0
.fill_table_0:
    mov ecx, edi
    or ecx, 3
    mov dword [(table0)+esi*4], ecx
    add edi, 4096
    inc esi
    cmp esi, 1024
    jne .fill_table_0

    mov esi, 0x0
    mov edi, 0x100000
.fill_table_768:
    mov ecx, edi
    or ecx, 3
    mov dword [(table768)+esi*4], ecx
    add edi, 4096
    inc esi
    cmp esi, 1024
    jne .fill_table_768

    mov ecx, table0
    ;sub ecx, 0xC0000000
    mov dword [(initial_page_dir)], ecx

    mov ecx, table768
    ;sub ecx, 0xC0000000
    mov dword [(initial_page_dir) + 768 * 4], ecx

    mov ecx, (initial_page_dir)
    mov cr3, ecx

    mov ecx, cr0
    or ecx, 0x80000001
    mov cr0, ecx

    jmp higher_half

section .text
bits 32
extern kmain
higher_half:
    mov esp, stack_top
    push ebx
    push eax
    call kmain
    
halt:
    hlt
    JMP halt

section .bss
align 4096
global initial_page_dir
initial_page_dir:
    resb 4096

align 4096
table0:
    resb 4096
    
align 4096
table768:
    resb 4096