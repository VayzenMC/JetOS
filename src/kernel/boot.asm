; JetOS Multiboot entry point
[BITS 32]

section .multiboot
align 4
    dd 0x1BADB002
    dd 0x00000003
    dd -(0x1BADB002 + 0x00000003)

section .text
global start
extern kmain

start:
    push ebx
    push eax
    call kmain

.hang:
    cli
    hlt
    jmp .hang