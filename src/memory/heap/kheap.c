#include "kheap.h"
#include "heap.h"
#include "config.h"
#include "kernel.h"
#include "memory/memory.h"

struct heap kernel_heap; //Made of the table and the start address of the heap
struct heap_table kernel_heap_table; //Contains 4096B entries and size of the same table

void kheap_init()
{
    int total_table_entries = CROSOS_HEAP_SIZE_BYTES / CROSOS_HEAP_BLOCK_SIZE;
    kernel_heap_table.entries =  (HEAP_BLOCK_TABLE_ENTRY*) CROSOS_HEAP_TABLE_ADDRESS; //Allocating the table at the address CROSOS_HEAP_TABLE_ADDRESS
    kernel_heap_table.total = total_table_entries; // Struct contents defined. The entries need to be initialized

    void* end = (void*) CROSOS_HEAP_ADDRESS + CROSOS_HEAP_SIZE_BYTES; //Last address of the heap
    int res = heap_create(&kernel_heap, (void*) (CROSOS_HEAP_ADDRESS), end, &kernel_heap_table);
    if (res < 0)
    {
        print("Failed to create heap\n");
    }
}

//Allocate a given size to the heap
void* kmalloc(size_t size) 
{
    return heap_malloc(&kernel_heap, size);
}

//Allocate a given size to the heap and initialize it to 0s.
void* kzalloc(size_t size) 
{
    void* ptr = kmalloc(size);
    if(!ptr)
    {
        return 0;
    }
    memset(ptr, 0x00, size);
    return ptr;

}

//Free a given heap sector
void kfree(void* ptr)
{
    heap_free(&kernel_heap, ptr);
}