#ifndef DISK_H
#define DISK_H
#include <stdint.h>
#include "fs/file.h"

typedef uint32_t CROSOS_DISK_TYPE;

#define CROSOS_DISK_TYPE_REAL 0;
struct disk
{
    CROSOS_DISK_TYPE type;
    uint32_t sector_size;
    uint32_t id;
    struct filesystem* filesystem;
    //Private data of the filesystem
    void* fs_private;
};
void disk_search_and_init();
struct disk* disk_get(uint32_t index);
uint32_t disk_read_block(struct disk* idisk, uint32_t lba, uint32_t total, void* buff);
#endif