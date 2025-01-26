global start
extern long_mode_start
extern set_up_paging

section .text
bits 32
start:
    call check_cpuid
    mov eax, 0x80000000    ; Set the A-register to 0x80000000.
    cpuid ; cpu identification
    cmp eax, 0x80000001    ; Compare the A-register with 0x80000001.
    jb .halt ; TODO

    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29 ; test if the lm bit, which is bit 29, is set in the d register
    jz .halt ; TODO

    call set_up_paging
    call load_page_directory
    call enable_paging
    
    cli

.halt:
    hlt
    jmp .halt

check_cpuid:
    pushfd                               ;Save EFLAGS
    pushfd                               ;Store EFLAGS
    xor dword [esp],0x00200000           ;Invert the ID bit in stored EFLAGS
    popfd                                ;Load stored EFLAGS (with ID bit inverted)
    pushfd                               ;Store EFLAGS again (ID bit may or may not be inverted)
    pop eax                              ;eax = modified EFLAGS (ID bit may or may not be inverted)
    xor eax,[esp]                        ;eax = whichever bits were changed
    popfd                                ;Restore original EFLAGS
    and eax,0x00200000                   ;eax = zero if ID bit can't be changed, else non-zero
    ret

load_page_directory:
    push ebp
    mov esp, ebp
    mov 8(esp), eax
    mov eax, cr3
    mov ebp, esp
    pop ebp
    ret

enable_paging:
    push ebp
    mov esp, ebp
    mov cr0, eax
    or 0x80000000, eax
    mov eax, cr0
    mov ebp, esp
    pop ebp
    ret