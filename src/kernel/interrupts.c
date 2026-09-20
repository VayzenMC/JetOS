#include "interrupts.h"

struct idt_entry {
    unsigned short offset_low;
    unsigned short selector;
    unsigned char zero;
    unsigned char flags;
    unsigned short offset_high;
} __attribute__((packed));

struct idt_pointer {
    unsigned short limit;
    unsigned int base;
} __attribute__((packed));

extern void interrupt_default(void);
extern void interrupt_error_code(void);
static struct idt_entry idt[256];

static void set_gate(unsigned int number, void (*handler)(void))
{
    unsigned int address = (unsigned int)handler;
    idt[number].offset_low = (unsigned short)address;
    idt[number].selector = 0x08;
    idt[number].zero = 0;
    idt[number].flags = 0x8E;
    idt[number].offset_high = (unsigned short)(address >> 16);
}

static void outb(unsigned short port, unsigned char value)
{
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

void interrupts_init(void)
{
    struct idt_pointer pointer;
    unsigned int i;

    __asm__ volatile("cli");
    for (i = 0; i < 256; ++i) set_gate(i, interrupt_default);
    set_gate(8, interrupt_error_code);
    set_gate(10, interrupt_error_code);
    set_gate(11, interrupt_error_code);
    set_gate(12, interrupt_error_code);
    set_gate(13, interrupt_error_code);
    set_gate(14, interrupt_error_code);
    set_gate(17, interrupt_error_code);
    pointer.limit = sizeof(idt) - 1;
    pointer.base = (unsigned int)idt;
    __asm__ volatile("lidt %0" : : "m"(pointer));

    /* Remap legacy PIC IRQs away from CPU exception vectors. */
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0xFE); /* keyboard is left masked until a real handler exists */
    outb(0xA1, 0xFF);
     /* The current shell polls the keyboard, so leave hardware IRQs disabled
         until device-specific handlers are installed. */
}