OUTPUT_FORMAT(elf32-i386)
ENTRY(shell_main)

SECTIONS
{
    . = 0x40000000; /* Shell user virtual address space'da ishlaydi. */
    .text : { *(.text) }
    .rodata : { *(.rodata) }
    .data : { *(.data) }
    .bss : { *(COMMON) *(.bss) }
}