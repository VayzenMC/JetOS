#include "vm.h"

#define PAGE_PRESENT 0x001U
#define PAGE_WRITABLE 0x002U
#define PAGE_USER 0x004U
#define PAGE_TABLE_COUNT 64

static unsigned int *page_directory = (unsigned int *)0x00009000;
static unsigned int *page_tables = (unsigned int *)0x0000A000;
static unsigned int next_table;
static unsigned int next_frame = 0x01000000;
static unsigned int next_virtual = VM_USER_BASE;

static void zero_page(unsigned int *page)
{
    unsigned int i;
    for (i = 0; i < 1024; ++i) page[i] = 0;
}

static unsigned int *get_page_table(unsigned int virtual_address,
                                    unsigned int user, unsigned int writable)
{
    unsigned int directory_index = virtual_address >> 22;
    unsigned int table_index = (page_directory[directory_index] & 0xFFFFF000U);
    unsigned int flags = PAGE_PRESENT | (writable ? PAGE_WRITABLE : 0)
                       | (user ? PAGE_USER : 0);

    if (table_index == 0) {
        if (next_table >= PAGE_TABLE_COUNT) return 0;
        table_index = (unsigned int)&page_tables[next_table * 1024];
        ++next_table;
        zero_page((unsigned int *)table_index);
        page_directory[directory_index] = table_index | flags;
    } else {
        page_directory[directory_index] |= flags;
    }
    return (unsigned int *)table_index;
}

void vm_init(void)
{
    unsigned int i;
    unsigned int address;

    next_table = 0;
    next_frame = 0x01000000;
    next_virtual = VM_USER_BASE;
    zero_page(page_directory);

    /* Keep the kernel, VGA memory and boot data identity mapped. */
    for (i = 0; i < 4; ++i) {
        unsigned int *table = get_page_table(i << 22, 0, 1);
        if (!table) return;
        for (address = 0; address < 0x00400000; address += VM_PAGE_SIZE)
            table[address >> 12] = (i << 22) + address | PAGE_PRESENT | PAGE_WRITABLE;
    }
    __asm__ volatile("mov %0, %%cr3\n\t"
                     "mov %%cr0, %%eax\n\t"
                     "or $0x80000000, %%eax\n\t"
                     "mov %%eax, %%cr0" : : "r"(page_directory) : "eax", "memory");
}

int vm_map_page(unsigned int virtual_address, unsigned int physical_address,
                unsigned int writable, unsigned int user)
{
    unsigned int *table;
    unsigned int index;

    if ((virtual_address & (VM_PAGE_SIZE - 1)) != 0
        || (physical_address & (VM_PAGE_SIZE - 1)) != 0)
        return -1;
    table = get_page_table(virtual_address, user, writable);
    if (!table) return -1;
    index = (virtual_address >> 12) & 0x3FF;
    if (table[index] & PAGE_PRESENT) return -1;
    table[index] = physical_address | PAGE_PRESENT
                 | (writable ? PAGE_WRITABLE : 0)
                 | (user ? PAGE_USER : 0);
    __asm__ volatile("invlpg (%0)" : : "r"(virtual_address) : "memory");
    return 0;
}

static void vm_clear_page(unsigned int virtual_address)
{
    unsigned int directory_index = virtual_address >> 22;
    unsigned int table_address = page_directory[directory_index] & 0xFFFFF000U;
    unsigned int *table;
    unsigned int index;

    if (table_address == 0) return;
    table = (unsigned int *)table_address;
    index = (virtual_address >> 12) & 0x3FF;
    table[index] = 0;
    __asm__ volatile("invlpg (%0)" : : "r"(virtual_address) : "memory");
}

void *vm_alloc_pages(unsigned int page_count)
{
    unsigned int virtual_address = next_virtual;
    unsigned int first_frame = next_frame;
    unsigned int i;

    if (page_count == 0 || page_count > 0x100000U / VM_PAGE_SIZE
        || virtual_address > 0xC0000000U - page_count * VM_PAGE_SIZE)
        return 0;
    for (i = 0; i < page_count; ++i) {
        if (vm_map_page(virtual_address + i * VM_PAGE_SIZE, next_frame,
                        1, 1) < 0) {
            while (i != 0) {
                --i;
                vm_clear_page(virtual_address + i * VM_PAGE_SIZE);
            }
            next_frame = first_frame;
            return 0;
        }
        next_frame += VM_PAGE_SIZE;
    }
    next_virtual += page_count * VM_PAGE_SIZE;
    return (void *)virtual_address;
}