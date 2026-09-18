#include "iso9660.h"

typedef void (*shell_entry_t)(void);

#define DIRECTORY_BUFFER ((unsigned char *)0xA00000)

static void screen_clear(void)
{
    unsigned char *video_memory = (unsigned char *)0xB8000;
    int i;

    for (i = 0; i < 80 * 25 * 2; i += 2) {
        video_memory[i] = ' ';
        video_memory[i + 1] = 0x07;
    }
}

static void print_at(const char *message, int row, unsigned char color)
{
    unsigned char *video_memory = (unsigned char *)0xB8000;
    int i = 0;

    while (message[i] != '\0' && i < 80) {
        video_memory[(row * 80 + i) * 2] = message[i];
        video_memory[(row * 80 + i) * 2 + 1] = color;
        ++i;
    }
}

void kmain(unsigned int magic, unsigned int addr)
{
    unsigned char *shell = (unsigned char *)0x200000;
    int shell_size;

    (void)addr;
    screen_clear();
    if (magic != 0x2BADB002) {
        print_at("JetOS: invalid multiboot magic", 0, 0x0C);
        for (;;) __asm__ volatile("hlt");
    }

    print_at("JetOS: loading /BOOT/SHELL.BIN from ISO", 0, 0x0A);
    iso9660_list_directory("/BOOT", DIRECTORY_BUFFER, 4096);
    shell_size = iso9660_read_file("/BOOT/SHELL.BIN", shell, 0x100000);
    if (shell_size < 0) {
        print_at("JetOS: /BOOT/SHELL.BIN not found in ISO", 1, 0x0C);
        for (;;) __asm__ volatile("hlt");
    }

    ((shell_entry_t)shell)();
    print_at("JetOS: shell returned", 1, 0x0E);
    for (;;) __asm__ volatile("hlt");
}