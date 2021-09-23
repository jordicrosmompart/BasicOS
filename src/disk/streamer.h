#ifndef DISKSTREAMER_H
#define DISKSTREAMER_H

#include "disk.h"
#include <stdint.h>

struct disk_stream
{
    uint32_t pos; //Position of the disk that is wanted to be read
    struct disk* disk; // Reference to the disk used
};

struct disk_stream* diskstreamer_new(uint32_t disk_id);
uint32_t diskstreamer_seek(struct disk_stream* stream, uint32_t pos);
uint32_t diskstreamer_read(struct disk_stream* stream, void* out, uint32_t total);
void diskstreamer_close(struct disk_stream* stream);

#endif