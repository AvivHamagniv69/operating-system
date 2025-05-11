bits 32

section .multiboot_header
align 4
    dd 0x1BADB002          ; Magic number (Multiboot1)
    dd 0x00000003          ; Flags: align modules + memory info
    dd -(0x1BADB002 + 0x00000003) ; Checksum (must make sum == 0)

section .boot
global start

section .text
    align 4

extern kmain

start:
    cli
    mov esp, stack_top
    push ebx
    ; push eax
    call kmain
    hlt
.halt:
    cli
    hlt
    jmp .halt

section .bss
    align 16
    stack_bottom:
        resb 4096
    stack_top: