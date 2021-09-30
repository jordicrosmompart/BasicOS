#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define PAGING_CACHE_DISABLED 0b00010000
#define PAGING_WRITE_THROUGH 0b00001000
#define PAGING_ACCESS_FROM_ALL 0b00000100 // Accessed from all rings or only kernel
#define PAGING_IS_WRITABLE 0b00000010 // Writeable page
#define PAGING_IS_PRESENT 0b00000001 // Page exists (addressable to physical memory)

#define PAGING_TOTAL_ENTRIES_PER_TABLE 1024
#define PAGING_PAGE_SIZE 4096

//Struct that represents the page directory for the kernel
struct paging_4gb_chunk
{
    uint32_t* directory_entry; 
};

void paging_switch(struct paging_4gb_chunk* directory);
struct paging_4gb_chunk* paging_new_4gb(uint8_t flags);
void enable_paging();

bool paging_is_aligned(void* address);
uint32_t paging_set(uint32_t* directory, void* virt, uint32_t val);

void paging_free_4gb(struct paging_4gb_chunk* chunk);
uint32_t* paging_4gb_chunk_get_directory(struct paging_4gb_chunk* chunk);

int32_t paging_map_to(struct paging_4gb_chunk* directory, void* virt, void* phys, void* phys_end, int32_t flags);
int32_t paging_map_range(struct paging_4gb_chunk* directory, void* virt, void* phys, int32_t count, int32_t flags);
int32_t paging_map(struct paging_4gb_chunk* directory, void* virt, void* phys, int32_t flags);
void* paging_align_address(void* ptr);
uint32_t paging_get(uint32_t* directory, void* virt);
void* paging_align_to_lower_page(void* addr);
void* paging_get_physical_address(uint32_t* directory, void* virt);


#endif