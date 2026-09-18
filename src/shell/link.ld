OUTPUT_FORMAT(elf32-i386)
ENTRY(shell_main)

SECTIONS
{
    . = 2M; /* Kernel 1MB da, shell esa 2MB da joylashadi */
    .text : { *(.text) }
    .rodata : { *(.rodata) }
    .data : { *(.data) }
    .bss : { *(COMMON) *(.bss) }
}