#ifndef FILE_H
#define FILE_H
#include <stdint.h>
#include "pparser.h"


typedef uint32_t FILE_SEEK_MODE;
enum
{
    SEEK_SET,
    SEEK_CUR,
    SEEK_END
};

typedef uint32_t FILE_MODE;
enum 
{
    FILE_MODE_READ,
    FILE_MODE_WRITE,
    FILE_MODE_APPEND,
    FILE_MODE_INVALID
};

enum
{
    FILE_STAT_READ_ONLY = 0b00000001,
};

typedef uint32_t FILE_STAT_FLAGS;
struct file_stat
{
    FILE_STAT_FLAGS flags;
    uint32_t filesize;
};

struct disk;
//Function pointer
typedef void* (*FS_OPEN_FUNCTION) (struct disk* disk, struct path_part* path, FILE_MODE mode);
typedef uint32_t (*FS_RESOLVE_FUNCTION) (struct disk* disk);
typedef uint32_t (*FS_SEEK_FUNCTION) (void* private, uint32_t offset, FILE_SEEK_MODE seek_mode);
typedef uint32_t (*FS_READ_FUNCTION) (struct disk* disk, void* private, uint32_t size, uint32_t nmemb, char* out);
typedef uint32_t (*FS_CLOSE_FUNCTION) (void* private);
typedef uint32_t (*FS_STAT_FUNCTION) (struct disk* disk, void* private, struct file_stat* stat);

struct filesystem
{
    //Filesystem should return zero from resolve if the provided disk is using its filesystem
    FS_RESOLVE_FUNCTION resolve; // Function pointer of every filesystem to guess if the disk is using this filesystem
    FS_OPEN_FUNCTION open; // Open function according to a given filesystem
    FS_READ_FUNCTION read; //Reads the file loaded with open
    FS_SEEK_FUNCTION seek; // Sets the pointer to a given position in the file
    FS_STAT_FUNCTION stat; // Gets flags and size of the file
    FS_CLOSE_FUNCTION close; //Closes the file
    char name[20]; // Name of the filesystem, i.e. FAT16 or NTFS
};

struct file_descriptor
{
    //The descriptor index
    uint32_t index;
    struct filesystem* filesystem; //Filesystem of the file

    //Private data for internal file descriptor
    void* private; // The filesystem driver will use it to identify the file in its filesystem domain

    //Disk that the file descriptor should use
    struct disk* disk;
};

void fs_init();
uint32_t fopen(const char* filename, const char* mode_string); //Open file function
uint32_t fread(void* ptr, uint32_t size, uint32_t nmemb, uint32_t fd); //Read contents of the file
uint32_t fclose(uint32_t fd); //Close the file, freeing the descriptors loaded
uint32_t fseek(uint32_t fd, uint32_t offset, FILE_SEEK_MODE whence); // Set the pointer position in the file
uint32_t fstat(uint32_t fd, struct file_stat* stat); //Get the stat flags of the file and its size
void fs_insert_filesystem(struct filesystem* filesystem); // Add new filesystem to the supported filesystems of the OS
struct filesystem* fs_resolve(struct disk* disk); //Resolve filesystem for a given disk
#endif