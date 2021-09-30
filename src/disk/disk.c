#include "io/io.h"
#include "disk.h"
#include "memory/memory.h"
#include "config.h"
#include "status.h"

struct disk disk;

//Same commands commented at boot.asm
//The disk read is the main one, there is no implementation for other secondary disks
uint32_t disk_read_sector(uint32_t lba, uint32_t total, void* buff)
{
    outb(0x1F6, (lba >> 24) | 0xE0);
    outb(0x1F2, total);
    outb(0x1F3, (unsigned char) (lba & 0xff));
    outb(0x1F4, (unsigned char) (lba >> 8));
    outb(0x1F5, (unsigned char) (lba >> 16));
    outb(0x1F7, 0x20);

    unsigned short* ptr = (unsigned short*) buff;
    for(int b = 0; b < total; b++)
    {
        //Wait for the buffer to be ready
        char c = insb(0x1F7); //Read from the bus
        while(!(c & 0x08))
        {
            c = insb(0x1F7); //Keep reading flag, when the flag is set, the contents can be read
        }

        //Copy from hard disk to memory
        for(int i = 0; i < 256; i++)
        {
            *ptr = insw(0x1F0); //Reads the contents and stores them in the ptr that points to buff
            ptr++;
        }

    }
    return 0;
}

//Initializes a disk
void disk_search_and_init()
{
    memset(&disk, 0, sizeof(disk)); //Sets space in memory for the information of the used disk
    disk.type = CROSOS_DISK_TYPE_REAL;
    disk.sector_size = CROSOS_SECTOR_SIZE;
    disk.id = 0;
    disk.filesystem = fs_resolve(&disk); //Gets the filesystem of the disk
}

//Returns a disk given an index (currently only 0)
struct disk* disk_get(uint32_t index) 
{
    if (index != 0) //Only disk0 is implemented
    {
        return 0;
    }
    return &disk;
}

//Reads a disk block by an 'lba' given
uint32_t disk_read_block(struct disk* idisk, uint32_t lba, uint32_t total, void* buff)
{
    if(idisk != &disk) //The only disk used is the global variable of this file
    {
        return -EIO;
    }

    return disk_read_sector(lba, total, buff); // Read sector for the function
}