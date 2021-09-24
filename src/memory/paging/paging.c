#include "paging.h"
#include "memory/heap/kheap.h"
#include "status.h"


extern void paging_load_directory(uint32_t* directory);

static uint32_t* current_directory = 0;

//Create new page directory and tables to manage 4GB of memory
//The addresses stores on the tables are linear. This means that there is no mapping from the directory + table entries
//Directory entry 0 table entry 0 points to the address 0
//Directory entry 4 table entry 3 points to the addres 0x1000000 + 0x3000 = 0x1003000. NO TRICKS, just in order.
struct paging_4gb_chunk* paging_new_4gb(uint8_t flags)
{
    uint32_t* directory = kzalloc(sizeof(uint32_t) * PAGING_TOTAL_ENTRIES_PER_TABLE); //Allocate all the page directory
    int offset = 0;
    for(int i = 0; i < PAGING_TOTAL_ENTRIES_PER_TABLE; i++) 
    {
        uint32_t* entry = kzalloc(sizeof(uint32_t) * PAGING_TOTAL_ENTRIES_PER_TABLE); //Allocate every page table
        for(int b = 0; b < PAGING_TOTAL_ENTRIES_PER_TABLE; b++) 
        {
            entry[b] = (offset + (b * PAGING_PAGE_SIZE)) | flags; //Assign physical address to every page table entry, considering the flags
        }
        offset += PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE; //Increase offset for the following table physical addres, which is incremented
        directory[i] = (uint32_t) entry | flags | PAGING_IS_WRITABLE; // Saves the created page table to the page directory, including the flags. The flags may specify that every table entry is not writable, but we want to enforce that every emtry in the page directory is writable
    }

    struct paging_4gb_chunk* chunk_4gb = kzalloc(sizeof(struct paging_4gb_chunk)); //Allocate the struct that points to the page directory address
    chunk_4gb->directory_entry = directory; // Save the page directory address
    return chunk_4gb;
}

void paging_switch(struct paging_4gb_chunk* directory)
{
    paging_load_directory(directory->directory_entry); //Load to cr3 register the address of the page directory
    current_directory = directory->directory_entry; // Update the current directory, for when it is changed
}

//Frees all the page directory
void paging_free_4gb(struct paging_4gb_chunk* chunk)
{
    for(uint32_t i = 0; i < 1024; i++)
    {
        uint32_t entry = chunk->directory_entry[i]; //Gets the directory entry
        uint32_t* table = (uint32_t*) (entry & 0xfffff000); //Gets the table address
        kfree(table); //Frees the table
    }

    kfree(chunk->directory_entry); //Frees the directory entries
    kfree(chunk); //Frees the whole chunk
}

uint32_t* paging_4gb_chunk_get_directory(struct paging_4gb_chunk* chunk) 
{
    return chunk->directory_entry; // Returns the address of the paging directory
}

bool paging_is_aligned(void* address)
{
    return ((uint32_t) address % PAGING_PAGE_SIZE)== 0; //Checks that the address provided is aligned
}

//Resolves a virtual address to the page directory entry and the page table entry
uint32_t paging_get_indexes(void* virtual_address, uint32_t* directory_index_out, uint32_t* table_index_out)
{
    uint32_t res = 0;
    if(!paging_is_aligned(virtual_address)) //Check that the virtual address is aligned
    {
        res = -EINVARG;
        goto out;
    }

    *directory_index_out = ((uint32_t) virtual_address / (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE)); //Gets the entry of the page directory
    *table_index_out = ((uint32_t) virtual_address % (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE) / PAGING_PAGE_SIZE); //Gets the entry of the page table

out:
    return res;
}

//Aligns a given address to the following address that is aligned
void* paging_align_address(void* ptr)
{
    if((uint32_t) ptr % PAGING_PAGE_SIZE)
    {
        return (void*) ((uint32_t) ptr + PAGING_PAGE_SIZE - ((uint32_t) ptr % PAGING_PAGE_SIZE));
    }

    return ptr;
}

void* paging_align_to_lower_page(void* addr)
{
    uint32_t _addr = (uint32_t) addr;
    _addr -= (_addr % PAGING_PAGE_SIZE);
    return (void*) _addr;
}

int32_t paging_map(struct paging_4gb_chunk* directory, void* virt, void* phys, int32_t flags)
{
    if(((uint32_t) virt % PAGING_PAGE_SIZE) || ((uint32_t) phys % PAGING_PAGE_SIZE))
    {
        return -EINVARG; //Both virtual and physical address should be aligned to work correctly on paging
    }

    return paging_set(directory->directory_entry, virt, (uint32_t) phys | flags);
}

int32_t paging_map_range(struct paging_4gb_chunk* directory, void* virt, void* phys, int32_t count, int32_t flags)
{
    int32_t res = 0;
    for(int32_t i = 0; i < count; i++) //Iteration for every page to map
    {
        res = paging_map(directory, virt, phys, flags); //Map to a directory the current 'virt' and 'phys' addresses of the iteration
        if(res < 0)
            break;
        virt += PAGING_PAGE_SIZE; //Increase the virtual addres by a page size
        phys += PAGING_PAGE_SIZE; //Same for the mapped physical address
    }

    return res;
}

int32_t paging_map_to(struct paging_4gb_chunk* directory, void* virt, void* phys, void* phys_end, int32_t flags)
{
    int32_t res = 0;
    if((uint32_t) virt % PAGING_PAGE_SIZE)
    {
        res = EINVARG;
        goto out;
    }

    if((uint32_t) phys % PAGING_PAGE_SIZE)
    {
        res = EINVARG;
        goto out;
    }

    if((uint32_t) phys_end % PAGING_PAGE_SIZE)
    {
        res = EINVARG;
        goto out;
    }

    if((uint32_t) phys_end < (uint32_t) phys)
    {
        res = EINVARG;
        goto out;
    }

    uint32_t total_bytes = phys_end - phys;
    uint32_t total_pages = total_bytes / PAGING_PAGE_SIZE; //Sets number of pages to be mapped
    res = paging_map_range(directory, virt, phys, total_pages, flags);

out:
    return res;
}

//Sets a table entry, mapping the address of the 'val' to the new virtual address 'virt'. This breaks the linearility set by default when the page directories and tables of the kernel are created
uint32_t paging_set(uint32_t* directory, void* virt, uint32_t val)
{
    if(!paging_is_aligned(virt))
    {
        return -EINVARG;
    }

    uint32_t directory_index = 0;
    uint32_t table_index = 0;
    int res = paging_get_indexes(virt, &directory_index, &table_index); //Get directory and table indexes from the virtual address
    if(res < 0)
    {
        return res;
    }

    uint32_t entry = directory[directory_index]; //Get directory entry
    uint32_t* table = (uint32_t*) (entry & 0xfffff000); // Ignoring the flags of the entry and keep the address of the table
    table[table_index] = val; // Set the address + flags to the pagins table

    return 0;
}

//Returns the physical address for 'virt' of the 'directory'
uint32_t paging_get(uint32_t* directory, void* virt)
{
    uint32_t directory_index = 0;
    uint32_t table_index = 0;
    paging_get_indexes(virt, &directory_index, &table_index); //Get directory and table indexes
    uint32_t entry = directory[directory_index]; //Get entry of the directory
    uint32_t* table = (uint32_t*) (entry & 0xFFFFF000); //Get table addres by ignoring flags
    return table[table_index]; //Return table entry for the 'virt'
}