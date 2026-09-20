[BITS 32]

section .text
global interrupt_default
global interrupt_error_code

interrupt_default:
    pushad
    mov al, 0x20
    out 0x20, al
    popad
    iretd

interrupt_error_code:
    pushad
    popad
    add esp, 4
    iretd