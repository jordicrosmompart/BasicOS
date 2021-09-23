#include "heap.h"
#include "kernel.h"
#include "status.h"
#include "memory/memory.h"
#include <stdbool.h>


//Validates that the number of entries of the created table corresponds to the size of the heap
static int heap_validate_table(void* ptr, void* end, struct heap_table* table) 
{
    int res = 0;

    size_t table_size = (size_t) (end - ptr);
    size_t total_blocks = table_size / CROSOS_HEAP_BLOCK_SIZE;
    if(table->total != total_blocks) 
    {
        res = -EINVARG;
        goto out;
    }

out: 
    return res;
}

//Validates that the pointer provided is aligned with 4096B
static bool heap_validate_alignment(void* ptr) 
{
    return ((unsigned int) ptr % CROSOS_HEAP_BLOCK_SIZE) == 0;
}

//Create a new heap
//heap is the struct that holds its table and start address
//ptr is the origin address of the heap
//end is the last address of the heap (saddr + size)
//table is the heap table, already placed in memory
int heap_create(struct heap* heap, void* ptr, void* end, struct heap_table* table)
{
    int res = CROSOS_ALL_OK;

    if(!heap_validate_alignment(ptr) || !heap_validate_alignment(end))
    {
        res = -EINVARG;
        goto out;
    }

    //Clear heap struct
    memset(heap, 0, sizeof(struct heap));
    //Set base address and table reference
    heap->saddr = ptr;
    heap->table = table;

    res = heap_validate_table(ptr, end, table);
    if(res < 0)
    {
        goto out;
    }

    //Initialized the table contents to all free entries
    size_t table_size = sizeof(HEAP_BLOCK_TABLE_ENTRY) * table->total;
    memset(table->entries, HEAP_BLOCK_TABLE_ENTRY_FREE, table_size);

out:
    return res;
}

//Aligns the byte needed to be reserved with 4096B
static uint32_t heap_align_value_to_upper(uint32_t val) 
{
    if(val % CROSOS_HEAP_BLOCK_SIZE == 0)
    {
        return val;
    }

    val = (val - (val % CROSOS_HEAP_BLOCK_SIZE));
    val += CROSOS_HEAP_BLOCK_SIZE;
    return val;
}

static int heap_get_entry_type(HEAP_BLOCK_TABLE_ENTRY entry) 
{
    return entry & 0x0F; 
}

//Get start block by checking the flags on the entries of the table
uint32_t heap_get_start_block(struct heap* heap, uint32_t total_blocks)
{
    struct heap_table* table = heap->table;
    int bc = 0; // Current block number accumulated
    int bs = -1; //First free block when found

    for(size_t i = 0; i < table->total; i++)
    {
        if(heap_get_entry_type(table->entries[i]) != HEAP_BLOCK_TABLE_ENTRY_FREE)
        {
            //Entry occupied, go to next iteration and restart
            bc = 0;
            bs = -1;
            continue;
        }

        //If its free and this is the first block
        if(bs == -1) 
        {
            bs = i;
        }
        bc++;
        if(bc == total_blocks) // Enough blocks already
        {
            break;
        }

    }

    // Not found enough space
        if(bs == -1)
        {
            return -ENOMEM;
        }

        return bs;
}

void* heap_block_to_address(struct heap* heap, uint32_t block) 
{
    return heap->saddr + (block * CROSOS_HEAP_BLOCK_SIZE); //Base address + block address offset
}

//Mark the allocated blocks depending on their position in the reserved space
void heap_mark_blocks_taken(struct heap* heap, uint32_t start_block, uint32_t total_blocks)
{
    int end_block = (start_block + total_blocks) -1;

    HEAP_BLOCK_TABLE_ENTRY entry = HEAP_BLOCK_TABLE_ENTRY_TAKEN | HEAP_BLOCK_IS_FIRST; //First block
    if (total_blocks > 1) 
    {
        entry |= HEAP_BLOCK_HAS_NEXT; // If its not only 1 block, set the flag
    }

    for(int i= start_block; i<= end_block; i++) 
    {
        heap->table->entries[i] = entry; //Save previous iteration or first block
        entry = HEAP_BLOCK_TABLE_ENTRY_TAKEN;
        if(i != end_block -1) //If is the previous iteration to the last, we are setting the last block
        {
            entry |= HEAP_BLOCK_HAS_NEXT;
        }
    }
}

void* heap_malloc_blocks(struct heap* heap, uint32_t total_blocks)
{
    void* address = 0;

    int start_block = heap_get_start_block(heap, total_blocks);
    if(start_block < 0)
    {
        goto out; //No space in the heap
    }

    address = heap_block_to_address(heap, start_block); //Address of the first allocated block

    //Mark the blocks as taken
    heap_mark_blocks_taken(heap, start_block, total_blocks);

out:
    return address;
}

uint32_t heap_address_to_block(struct heap* heap, void* address)
{
    return ((uint32_t) (address - heap->saddr)) / CROSOS_HEAP_BLOCK_SIZE;
}

//Mark heap's table blocks as free
void heap_mark_blocks_free(struct heap* heap, uint32_t starting_block) 
{
    struct heap_table* table = heap->table;
    for(int i = starting_block; i < (uint32_t) table->total; i++) 
    {
        HEAP_BLOCK_TABLE_ENTRY entry = table->entries[i];
        table->entries[i] = HEAP_BLOCK_TABLE_ENTRY_FREE; // Clear the state back to entry free
        if(!(entry & HEAP_BLOCK_HAS_NEXT)) //Checking if it is last block
        {
            break;
        }
    }
}

//Alloc given size to the given heap
void* heap_malloc(struct heap* heap, size_t size)
{
    size_t aligned_size = heap_align_value_to_upper(size);
    uint32_t total_blocks = aligned_size / CROSOS_HEAP_BLOCK_SIZE;

    return heap_malloc_blocks(heap, total_blocks); //Alloc required blocks
}

void heap_free(struct heap* heap, void* ptr) 
{
    //It frees a group of blocks of the heap's table. The data in the heap is not cleared, it stays there as garbage
    heap_mark_blocks_free(heap, heap_address_to_block(heap, ptr));
}