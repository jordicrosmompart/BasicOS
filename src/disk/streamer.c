#include "streamer.h"
#include "memory/heap/kheap.h"
#include "config.h"
#include <stdbool.h>

//Creates a new streamer
struct disk_stream* diskstreamer_new(uint32_t disk_id)
{
    struct disk* disk = disk_get(disk_id);
    if(!disk)
    {
        return 0;
    }

    struct disk_stream* streamer = kzalloc(sizeof(struct disk_stream));
    streamer->pos = 0; //Position initialized to 0
    streamer->disk = disk; //Disk found by the disk_id
    return streamer;
}

//Sets the streamer to a given offset
uint32_t diskstreamer_seek(struct disk_stream* stream, uint32_t pos)
{
    stream->pos = pos; //Set position to start searching
    return 0;
}

//This function has a bug, if there is an offset and the data to be read + offset is higher than MAX_SIZE, we will read random data
uint32_t diskstreamer_read(struct disk_stream* stream, void* out, uint32_t total)
{
    uint32_t sector = stream->pos / CROSOS_SECTOR_SIZE; //Get sector from 'pos'
    uint32_t offset = stream->pos % CROSOS_SECTOR_SIZE; //Get offset from 'pos'
    uint32_t total_to_read = total;
    bool overflow = (offset + total_to_read) >= CROSOS_SECTOR_SIZE;
    char buff[CROSOS_SECTOR_SIZE]; //Create a temporary buffer of the sector to be read
    
    if(overflow)
    {
        total_to_read -= (offset + total_to_read) - CROSOS_SECTOR_SIZE;
    }
    
    uint32_t res = disk_read_block(stream->disk, sector, 1, buff); //Read the entire sector to 'buff'

    if(res < 0)
    {
        goto out;
    }

    for(int i = 0; i < total_to_read; i++)
    {
        *(char*) out++ = buff[offset+i]; //Start copying from the offset stated in the position
    }

    //Adjust the stream
    stream->pos += total_to_read; //Update the position in the streamer
    if(overflow)
    {
        //If more than a sector has to be read, call again the function decreasing the total data that is needed to be read
        res = diskstreamer_read(stream, out, total-total_to_read);
    }

out:
    return res;
}

//Frees the streamer from allocated memory
void diskstreamer_close(struct disk_stream* stream) 
{
    kfree(stream);
}