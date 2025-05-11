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
    dd 0x1BADB002
    dd 0x00000003
    dd -(0x1BADB002 + 0x00000003)

extern kmain

start:
    cli
    mov esp, stack_top
    ; mov eax, 0x2BADB002
    mov eax, multiboot_magic
    mov ebx, multiboot_info
    push ebx
    push eax
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

section .data
    multiboot_magic:
        .long 0
    multiboot_info:
        .long 0