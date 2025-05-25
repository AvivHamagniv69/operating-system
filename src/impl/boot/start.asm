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
extern _kernel_start
extern _kernel_end
global _start
_start:
    ; EDI = table0 - 0xC0000000
    mov edi, table0 - 0xC0000000

    ; ESI = 0
    xor esi, esi

    ; ECX = 1023
    mov ecx, 1023

.loop:
    ; if (ESI < _kernel_start) jump to .skip
    cmp esi, _kernel_start
    jl .skip

    ; if (ESI >= _kernel_end - 0xC0000000) jump to .done
    cmp esi, _kernel_end - 0xC0000000
    jge .done

    ; EDX = ESI | 0x3
    mov edx, esi
    or edx, 0x3

    ; [EDI] = EDX
    mov [edi], edx

.skip:
    ; ESI += 4096
    add esi, 4096

    ; EDI += 4
    add edi, 4

    ; loop again (manual loop since ECX not used here for counting)
    jmp .loop

.done:
    ; table0[1023] = 0x000B8000 | 0x003
    mov dword [table0 - 0xC0000000 + 1023 * 4], 0x000B8003

    ; initial_page_dir[0] = (table0 - 0xC0000000) | 0x003
    mov dword [initial_page_dir - 0xC0000000 + 0], (table0 - 0xC0000000 + 0x003)

    ; initial_page_dir[768] = (table0 - 0xC0000000) | 0x003
    mov dword [initial_page_dir - 0xC0000000 + 768 * 4], (table0 - 0xC0000000 + 0x003)

    ; ECX = initial_page_dir - 0xC0000000
    mov ecx, initial_page_dir - 0xC0000000

    ; load CR3
    mov cr3, ecx

    ; ECX = CR0
    mov ecx, cr0

    ; set PG and PE bits (0x80000000 | 0x00010000)
    or ecx, 0x80010000

    ; write back to CR0
    mov cr0, ecx

    ; jump to label .enable_paging (flat mode continue execution)
    lea ecx, [higher_half]
    jmp ecx

section .text
extern kmain
higher_half:
    mov dword [initial_page_dir], 0
    mov ecx, cr3
    mov cr3, ecx
    mov esp, stack_top
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