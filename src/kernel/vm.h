#ifndef JETOS_VM_H
#define JETOS_VM_H

/* User virtual memory starts above the kernel identity-mapped area. */
#define VM_USER_BASE 0x40000000U
#define VM_PAGE_SIZE 0x1000U

void vm_init(void);
void *vm_alloc_pages(unsigned int page_count);
int vm_map_page(unsigned int virtual_address, unsigned int physical_address,
                unsigned int writable, unsigned int user);

#endif