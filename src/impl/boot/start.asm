bits 32

section .multiboot_header
header_start:
    ; magic number
    dd 0xe85250d6 ; multiboot 2
    ; architecture
    dd 0 ; protected mode i386
    ; header length
    dd header_end - header_start
    ; checksum
    dd 0x100000000 - (0xe85250d6 + 0 + (header_end - header_start))

    ; end tag
    dw 0
    dw 0
    dd 8

header_end:

section .boot
global start

section .text
    align 4
    dd 0x1badb002
    dd 0x00000000
    dd -(0x1badb002 + 0x00000000)

extern kmain

start:
    cli
    mov esp, stack_top
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
