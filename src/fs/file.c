#include "file.h"
#include "config.h"
#include "memory/memory.h"
#include "memory/heap/kheap.h"
#include "status.h"
#include "kernel.h"
#include "fat/fat16.h"
#include "disk/disk.h"
#include "string/string.h"

struct filesystem* filesystems[CROSOS_MAX_FILESYSTEMS]; //Filesystems supported by OS
struct file_descriptor* file_descriptors[CROSOS_MAX_FILE_DESCRIPTORS]; // File descriptors handled in the OS

//Returns an empty position of the filesystems array of the OS
static struct filesystem** fs_get_free_filesystem()
{
    uint32_t i = 0;
    for (i = 0; i < CROSOS_MAX_FILESYSTEMS; i++)
    {
        if(filesystems[i] == 0)
        {
            return &filesystems[i];
        }
    }
    return 0;
}

//Allows drivers to insert themselves a new filesystem
void fs_insert_filesystem(struct filesystem* filesystem)
{
    struct filesystem** fs;
    fs = fs_get_free_filesystem();
    if (filesystem == 0) //No spaces left
    {
        //Create panic function
        print("Problem inserting filesystem"); 
        while(1) {}
    }

    *fs = filesystem; 
}

//Statically load a filesystem to kernel, not dynamically. TODO
static void fs_static_load()
{
    fs_insert_filesystem(fat16_init());
}

//Memsets the available filesystems and loads them. TODO
void fs_load()
{
    memset(filesystems, 0, sizeof(filesystems));
    fs_static_load();
}

//Memsets the file_descriptors and calls fs_load(). TODO
void fs_init()
{
    memset(file_descriptors, 0, sizeof(file_descriptors));
    fs_load();
}

static void file_free_descriptor(struct file_descriptor* desc)
{
    file_descriptors[desc->index-1] = 0x00;
    kfree(desc);
}

static uint32_t file_new_descriptor(struct file_descriptor** desc_out)
{
    uint32_t res = -ENOMEM;
    //Loop through all possible file descriptors
    for(uint32_t i = 0; i < CROSOS_MAX_FILE_DESCRIPTORS; i++)
    {
        if(file_descriptors[i] == 0) //If the index is empty
        {
        struct file_descriptor* desc = kzalloc(sizeof(struct file_descriptor)); //Allocate a new file descriptor
        //Descriptors start at 1
        desc->index = i + 1;
        file_descriptors[i] = desc;
        *desc_out = desc; //Set the memory address of the new created file descriptor. (use of double pointer)
        res = 0;
        break;
        }
    }
    return res;
}

//Gets a descriptor by its index
static struct file_descriptor* file_get_descriptor(uint32_t fd)
{
    if (fd <= 0 || fd >= CROSOS_MAX_FILE_DESCRIPTORS)
    {
        return 0;
    }
    uint32_t index = fd-1; //All indexes have a +1 added (upper function). Here it is substracted
    return file_descriptors[index];
}

//Resolves a filesystem of a disk
struct filesystem* fs_resolve (struct disk* disk)
{
    struct filesystem* fs = 0;
    for (uint32_t i = 0; i < CROSOS_MAX_FILESYSTEMS; i++) //Iterate through all loaded filesystems in the OS
    {
        if(filesystems[i] != 0 && filesystems[i]->resolve(disk) == 0) //Existing filesystem and its resolve function returns 0 (meaning it is the filesystem of the disk)
        {
            fs = filesystems[i];
            break;
        }
    }

    return fs; //Returns found filesystem
}

FILE_MODE file_get_mode_by_string(const char* str)
{
    FILE_MODE mode = FILE_MODE_INVALID;
    if(strncmp(str, "r", 1) == 0)
    {
        mode = FILE_MODE_READ;
    }
    else if (strncmp(str, "w", 1) == 0)
    {
        mode = FILE_MODE_WRITE;
    }
    else if(strncmp(str, "a", 1) == 0)
    {
        mode = FILE_MODE_APPEND;
    }
    return mode;
}
//Opens a file.
uint32_t fopen(const char* filename, const char* mode_str)
{
    uint32_t res = 0;
    //Parse path to check disk and directories
    struct path_root* root_path = pathparser_parse(filename, NULL);
    if(!root_path)
    {
        res = -EINVARG;
        goto out;
    }

    if(!root_path->first) //Only the root drive, we need more
    {
        res = -EINVARG;
        goto out;
    }

    //Check that drive exists (only drive 0 implemented)
    struct disk* disk = disk_get(root_path->drive_no);
    if(!disk)
    {
        res = -EIO;
        goto out;
    }

    if(!disk->filesystem)
    {
        res = -EIO;
        goto out;
    }

    //Parse mode to provide it to the filesystem
    FILE_MODE mode = file_get_mode_by_string(mode_str);
    if(mode == FILE_MODE_INVALID)
    {
        res = -EINVARG;
        goto out;
    }

    //Up to this point we have the disk loaded to 'disk', the path parsed to the 'root_path' with the linked subdirectories in it and the mode of opening a file to 'mode'

    //Call the filesystem custom implementation of fopen and store the result at the 'descriptor_private_data'
    void* descriptor_private_data = disk->filesystem->open(disk, root_path->first, mode);
    if(ISERR(descriptor_private_data))
    {
        res = ERROR_I(descriptor_private_data);
        goto out;
    }

    //Create and load the file descriptor to the OS array
    struct file_descriptor* desc = 0;
    res = file_new_descriptor(&desc); //Stores the new descriptor to the OS array
    if(res < 0)
    {
        goto out;
    }
    //Load the contents of the new file descriptor
    desc->filesystem = disk->filesystem;
    desc->private = descriptor_private_data;
    desc->disk = disk;
    res = desc->index; //Returns the index of the descriptor to be further used (accessing the OS descriptor array)

out:

    if( (int32_t) res < 0) //fopen shouldnt return negative values
    {
        res = 0;
    }
    return res;
}

uint32_t fstat(uint32_t fd, struct file_stat* stat)
{
    uint32_t res = 0;
    struct file_descriptor* desc = file_get_descriptor(fd); //Get descriptor by index
    if(!desc)
    {
        res = -EIO;
        goto out;
    }

    res = desc->filesystem->stat(desc->disk, desc->private, stat); //Calls filesystem function

out:
    return res;
}

uint32_t fclose(uint32_t fd)
{
    uint32_t res = 0;
    struct file_descriptor* desc = file_get_descriptor(fd);
    if(!desc)
    {
        res = -EIO;
        goto out;
    }

    res = desc->filesystem->close(desc->private);
    if(res == CROSOS_ALL_OK)
    {
        file_free_descriptor(desc); //Frees the descriptor of the OS, the contents in the private descriptor arae freed inside the filesystem function
    }

out:
    return res;
}

uint32_t fseek(uint32_t fd, uint32_t offset, FILE_SEEK_MODE whence)
{
    uint32_t res = 0;
    struct file_descriptor* desc = file_get_descriptor(fd);
    if(!desc)
    {
        res = -EIO;
        goto out;
    }

    res = desc->filesystem->seek(desc->private, offset, whence); //Call filesystem function
    
out:
    return res;
}


//Reads a file
uint32_t fread(void* ptr, uint32_t size, uint32_t nmemb, uint32_t fd)
{
    uint32_t res = 0;
    if (size == 0 || nmemb == 0 || fd < 1) //Argument checks
    {
        res = -EINVARG;
        goto out;
    }

    struct file_descriptor* desc = file_get_descriptor(fd);
    if(!desc)
    {
        res = -EINVARG;
        goto out;
    }

    res = desc->filesystem->read(desc->disk, desc->private, size, nmemb, (char*) ptr); //Calls filesystem function

out:
    return res;
}