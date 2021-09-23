#include "fat16.h"
#include <stdint.h>
#include "status.h"
#include "string/string.h"
#include "disk/disk.h"
#include "disk/streamer.h"
#include "memory/memory.h"
#include "kernel.h"
#include "memory/heap/kheap.h"

#define CROSOS_FAT16_SIGNATURE 0x29
#define CROSOS_FAT16_FAT_ENTRY_SIZE 0x02
#define CROSOS_FAT16_BAD_SECTOR 0xFF7
#define CROSOS_FAT16_UNUSED 0x00

typedef uint32_t FAT_ITEM_TYPE;
#define FAT_ITEM_TYPE_DIRECTORY 0
#define FAT_ITEM_TYPE_FILE 1

//FAT directory entry attributes bitmask
#define FAT_FILE_READ_ONLY 0x01
#define FAT_FILE_HIDDEN 0x02
#define FAT_FILE_SYSTEM 0x04
#define FAT_FILE_VOLUME_LABEL 0x08
#define FAT_FILE_SUBDIRECTORY 0x10
#define FAT_FILE_ARCHIVED 0x20
#define FAT_FILE_DEVICE 0x40
#define FAT_FILE_RESERVED 0x80

//Headers of the boot sector. Extension part, which is optional
struct fat_header_extended
{
    uint8_t drive_number;
    uint8_t win_nt_bit;
    uint8_t signature;
    uint32_t volume_id;
    uint8_t volume_id_string[11];
    uint8_t system_id_string[8];
} __attribute__((packed));

//Headers of the boot sector. Mandatory part
struct fat_header
{
    uint8_t short_hmp_ins[3];
    uint8_t oem_identifier[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fat_copies;
    uint16_t root_dir_entries;
    uint16_t number_of_sectors;
    uint8_t media_type;
    uint16_t sectors_per_fat;
    uint16_t sector_per_track;
    uint16_t number_of_heads;
    uint32_t hidden_sectors;
    uint32_t sectors_big;
} __attribute__((packed));

//Wrapper of the headers
struct fat_h 
{
    struct fat_header primary_header;
    union fat_h_e
    {
        struct fat_header_extended extended_header;
    } shared;
};

//Represents a directory, whether a file or a directory
struct fat_directory_item
{
    uint8_t file_name[8];
    uint8_t ext[3];
    uint8_t attribute;
    uint8_t reserved;
    uint8_t creation_time_tenths_of_a_sec;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access;
    uint16_t high_16_bits_first_cluster;
    uint16_t last_mod_time;
    uint16_t last_mod_date;
    uint16_t low_16_bits_first_cluster;
    uint32_t file_size;
} __attribute__((packed));

//Groups all the directories, including in which sector it is stores
struct fat_directory
{
    struct fat_directory_item* item;
    uint32_t total;
    uint32_t sector_pos;
    uint32_t ending_sector_pos;

};

//An item may be a file or a directory. It includes the directory structure and its type
struct fat_item
{
    union
    {
        struct fat_directory_item* item;
        struct fat_directory* directory;    
    };
    FAT_ITEM_TYPE type;
};

//Descriptor of the item. Includes a position and all the info of the directory
struct fat_file_descriptor
{
    struct fat_item* item;
    uint32_t pos;
};

//Private headers for internal FAT16 routines
struct fat_private
{
    struct fat_h header;
    struct fat_directory root_directory;

    //Used to stream data clusters
    struct disk_stream* cluster_read_stream;
    //Used to stream the file allocation table
    struct disk_stream* fat_read_stream;
    //Used in situation where we stream the directory
    struct disk_stream* directory_stream;
};

uint32_t fat16_resolve(struct disk* disk);
void* fat16_open(struct disk* disk, struct path_part* path, FILE_MODE mode);
uint32_t fat16_read(struct disk* disk, void* descriptor, uint32_t size, uint32_t nmemb, char* out_ptr);
uint32_t fat16_seek(void* private, uint32_t offset, FILE_SEEK_MODE seek_mode);
uint32_t fat16_stat(struct disk* disk, void* private, struct file_stat* stat);
uint32_t fat16_close(void* private);

//Creates the FAT16 filesystem instance
struct filesystem fat16_fs = 
{
    .resolve = fat16_resolve,
    .open = fat16_open,
    .read = fat16_read,
    .seek = fat16_seek,
    .stat = fat16_stat,
    .close = fat16_close
}; 
struct filesystem* fat16_init() //Returns the instantiated struct
{
    strcpy(fat16_fs.name, "FAT16");
    return &fat16_fs;
}

//Initializes disk streamers for cluster reads, fat reads and directory reads
static void fat16_init_private(struct disk* disk, struct fat_private* private)
{
    memset(private, 0, sizeof(struct fat_private)); //Clears the contents pointed by 'private'
    private->cluster_read_stream = diskstreamer_new(disk->id);
    private->fat_read_stream = diskstreamer_new(disk->id);
    private->directory_stream = diskstreamer_new(disk->id);
}

uint32_t fat16_sector_to_absolute(struct disk* disk, uint32_t sector)
{
    return sector * disk->sector_size;
}

uint32_t fat16_get_total_items_for_directory(struct disk* disk, uint32_t directory_start_sector)
{
    struct fat_directory_item item;
    //Create and initlialize empty_item
    struct fat_directory_item empty_item;
    memset(&empty_item, 0, sizeof(empty_item));

    struct fat_private* fat_private = disk->fs_private;

    uint32_t res = 0;
    uint32_t i = 0;
    uint32_t directory_start_pos = directory_start_sector * disk->sector_size; //Absolut offset of the root directory
    //Set the directory stream to the position
    struct disk_stream* stream = fat_private->directory_stream;
    if(diskstreamer_seek(stream, directory_start_pos) != CROSOS_ALL_OK)
    {
        res = -EIO;
        goto out;
    }

    while(1) // Read each directory item
    {
        if(diskstreamer_read(stream, &item, sizeof(item)) != CROSOS_ALL_OK)
        {
            res = -EIO;
            goto out;
        }

        if(item.file_name[0] == 0x00) //End of iteration, blank record
        {
            //We are done
            break;
        }

        if(item.file_name[0] == 0xE5) //Item unused
        {
            continue;
        }

        i++;
    }

    res = i;
out:
    return res;
}

uint32_t fat16_get_root_directory(struct disk* disk, struct fat_private* fat_private, struct fat_directory* directory)
{
    uint32_t res = 0;
    struct fat_header* primary_header = &fat_private->header.primary_header; //Main header of the private section (the one in the bootloader)
    uint32_t root_dir_sector_pos = (primary_header->fat_copies * primary_header->sectors_per_fat) + primary_header->reserved_sectors; //Offset (in sectors) of the root directory
    uint32_t root_dir_entries = fat_private->header.primary_header.root_dir_entries; //Number of entries in the root directory
    uint32_t root_dir_size = (root_dir_entries * sizeof(struct fat_directory_item)); //Size of the root directory
    uint32_t total_sectors = root_dir_size / disk->sector_size; //Number of sectors of the root directory
    if(root_dir_size % disk->sector_size)
    {
        total_sectors +=1; //Add a sector if there is a remainder
    }
    //Count items in the root directory, either folders or files
    uint32_t total_items = fat16_get_total_items_for_directory(disk, root_dir_sector_pos);

    //Allocate space for the root directory
    struct fat_directory_item* dir = kzalloc(root_dir_size);
    if(!dir)
    {
        res = -ENOMEM;
        goto out;
    }

    //Get the directory streamer
    struct disk_stream* stream = fat_private->directory_stream;
    //Set the position of the sector we want to read
    if(diskstreamer_seek(stream, fat16_sector_to_absolute(disk, root_dir_sector_pos)) != CROSOS_ALL_OK)
    {
        res = -EIO;
        goto out;
    }

    //Read the contents of the sector of root directory
    if(diskstreamer_read(stream, dir, root_dir_size) != CROSOS_ALL_OK)
    {
        res = -EIO;
        goto out;
    }

    directory->item = dir; //Get the directory
    directory->total = total_items; //Total items
    directory->sector_pos = root_dir_sector_pos; //Begin sector
    directory->ending_sector_pos = root_dir_sector_pos + (root_dir_size / disk->sector_size); //Last sector
out:
    return res;
}

uint32_t fat16_resolve(struct disk* disk) //Figures if the disk is using FAT16
{
    uint32_t res = 0;
    struct fat_private* fat_private = kzalloc(sizeof(struct fat_private)); //Gets the private data from disk
    fat16_init_private(disk, fat_private); //Creates streamers and stores them in the 'fat_private' variable

    disk->fs_private = fat_private;
    disk->filesystem = &fat16_fs;

    struct disk_stream* stream = diskstreamer_new(disk->id); //Create new streamer
    if(!stream)
    {
        res = -ENOMEM;
        goto out;
    }
    //Reads the contents of the header in the boot sector to the '&fat_private->header'
    if(diskstreamer_read(stream, &fat_private->header, sizeof(fat_private->header)) != CROSOS_ALL_OK)
    {
        res = -EIO;
        goto out;
    }

    if(fat_private->header.shared.extended_header.signature != 0x29)
    {
        res = -EFSNOTUS;
    }

    //Gets the root directory of the FAT filesystem, from the headers already parsed to 'fat_private' and set it to '&fat_private->root_directory'
    if(fat16_get_root_directory(disk, fat_private, &fat_private->root_directory) != CROSOS_ALL_OK)
    {
        res = -EIO;
        goto out;
    }

out:
    if(stream)
    {
        diskstreamer_close(stream);
    }
    if(res < 0)
    {
        kfree(fat_private);
        disk->fs_private = 0;
    }
    return res;
}

//Copy the filename of the item structure to a string for further treatment
void fat16_to_proper_string(char** out, const char* in)
{
    while(*in != 0x00 && *in != 0x20) //While the end of the filename is not reached
    {
        **out = *in; //Copy value to string
        *out += 1; //Increment pointer position to next string element
        in += 1; //Increment the in pointer to get the next character
    }

    if(*in == 0x20)
    {
        **out = 0x00; //Finish the string with a null character, if the original finishes with a space
    }
}

//Gets the name of the directory item + possible extension to 'out'
void fat16_get_full_relative_filename(struct fat_directory_item* item, char* out, uint32_t max_len)
{
    memset(out, 0x00, max_len); //Initializes the 'out' string
    char *out_tmp = out;
    fat16_to_proper_string(&out_tmp, (const char*) item->file_name);
    if(item->ext[0] != 0x00 && item->ext[0] != 0x20) //If the extension of the item exists (different than a null character and a space)
    {
        //Item found
        *out_tmp++ = '.'; //Add the dot
        fat16_to_proper_string(&out_tmp, (const char*) item->ext); //Add the extension to the name
    }
}

//Creates an allocated copy of the 'item'
struct fat_directory_item* fat16_clone_directory_item(struct fat_directory_item* item, uint32_t size)
{
    
    struct fat_directory_item* item_copy = 0;
    if(size < sizeof(struct fat_directory_item))
    {
        return 0;
    }
    item_copy = kzalloc(size);
    if(!item_copy)
    {
        return 0;
    }

    memcpy(item_copy, item, size);

    return item_copy;
}

static uint32_t fat16_get_first_cluster(struct fat_directory_item* item)
{
    return (item->high_16_bits_first_cluster | item->low_16_bits_first_cluster);
}

static uint32_t fat16_cluster_to_sector(struct fat_private* private, uint32_t cluster)
{
    //Last position of the root_directory + cluster * sectors_per_cluster. The -2 comes from the definition of the FAT. Check manual
    return private->root_directory.ending_sector_pos + ((cluster - 2) * private->header.primary_header.sectors_per_cluster);
}

static uint32_t fat16_get_first_fat_sector(struct fat_private* private)
{
    return private->header.primary_header.reserved_sectors; //Returns the position of the FAT table
}

static uint32_t fat16_get_fat_entry(struct disk* disk, uint32_t cluster)
{
    uint32_t res = -1;
    struct fat_private* private = disk->fs_private;
    struct disk_stream* stream = private->fat_read_stream;
    if(!stream)
    {
        goto out;
    }
    //Get FAT table position in disk
    uint32_t fat_table_position = fat16_get_first_fat_sector(private) * disk->sector_size;
    //Set the streamer to the position where the cluster we want to read is
    res = diskstreamer_seek(stream, fat_table_position * (cluster * CROSOS_FAT16_FAT_ENTRY_SIZE));
    if(res < 0)
    {
        goto out;
    }

    uint16_t result = 0;
    //Read the cluster entry on the table (2 bytes)
    res = diskstreamer_read(stream, &result, sizeof(result));
    if(res < 0)
    {
        goto out;
    }

    res = result;

out:
    return res;
}

static uint32_t fat16_get_cluster_for_offset(struct disk* disk, uint32_t starting_cluster, uint32_t offset)
{
    uint32_t res = 0;
    struct fat_private* private = disk->fs_private;
    uint32_t size_of_cluster_bytes = private->header.primary_header.sectors_per_cluster * disk->sector_size;
    uint32_t cluster_to_use = starting_cluster;
    uint32_t clusters_ahead = offset / size_of_cluster_bytes; // If the offset is higher than a cluster size, we will have to look for another cluster than the starting one
    for(uint32_t i = 0; i < clusters_ahead; i++)
    {
        //Entry gives the next cluster to read in the FAT or a error flag
        uint32_t entry = fat16_get_fat_entry(disk, cluster_to_use); //Gets the entry on the FAT table of the cluster we want to read

        //Check that the sector read is correct
        if(entry == 0xFF8 || entry == 0xFFF)
        {
            //Last entry in the file
            res = -EIO;
            goto out;
        }

        // Sector is marked
        if(entry == CROSOS_FAT16_BAD_SECTOR)
        {
            res = -EIO;
            goto out;
        }

        //Reserved sectors?
        if(entry == 0xFF0 || entry == 0xFF6)
        {
            res = -EIO;
            goto out;
        }

        if(entry == 0x00)
        {
            res = -EIO;
            goto out;
        }

        cluster_to_use = entry; //Set sector to returned value or to the next iteration value. By this, we can follow the FAT map to get the cluster we need to read
    }

    res = cluster_to_use;

out:
    return res;
}

static uint32_t fat16_read_internal_from_stream(struct disk* disk, struct disk_stream* stream, uint32_t cluster, uint32_t offset, uint32_t total, void* out)
{
    uint32_t res = 0;
    struct fat_private* private = disk->fs_private;
    uint32_t size_of_cluster_bytes = private->header.primary_header.sectors_per_cluster * disk->sector_size; //Get cluster size
    uint32_t cluster_to_use = fat16_get_cluster_for_offset(disk, cluster, offset); //Considers offset to maybe increment the reading cluster
    if(cluster_to_use < 0) //No cluster found
    {
        res = cluster_to_use;
        goto out;
    }

    uint32_t offset_from_cluster = offset % size_of_cluster_bytes; //Gets the offset from the gotten cluster

    uint32_t starting_sector = fat16_cluster_to_sector(private, cluster_to_use); //Gets the sector from the cluster
    uint32_t starting_pos = (starting_sector * disk->sector_size) + offset_from_cluster; //Starting position for streamer
    uint32_t total_to_read = total > size_of_cluster_bytes ? size_of_cluster_bytes : total; //Max total to read is the size of a cluster.
    res = diskstreamer_seek(stream, starting_pos); //Sets the starting position to the streamer
    if(res != CROSOS_ALL_OK)
    {
        goto out;
    }

    res = diskstreamer_read(stream, out, total_to_read); //Reads the desired data of the sectors (2 max)
    if(res != CROSOS_ALL_OK)
    {
        goto out;
    }

    total -= total_to_read;
    if(total > 0) //Still left to read
    {
        //Repeat operation, modifying the offset, the 'out' and providing an already modified total
        res = fat16_read_internal_from_stream(disk, stream, cluster, offset + total_to_read, total, out + total_to_read);
    }

out:
    return res;
}

//Reads the contents from a cluster to the total size wanted to be read
static uint32_t fat16_read_internal(struct disk* disk, uint32_t stating_cluster, uint32_t offset, uint32_t total, void* out)
{
    struct fat_private* fs_private = disk->fs_private;
    struct disk_stream* stream = fs_private->cluster_read_stream; //Get the cluster reader stream
    //Return the reading of all the clusters that we need to read to access the file
    return fat16_read_internal_from_stream(disk, stream, stating_cluster, offset, total, out);

}

void fat16_free_directory(struct fat_directory* directory)
{
    if(!directory)
    {
        return;
    }

    if(directory->item)
    {
        kfree(directory->item);
    }

    kfree(directory);
}

void fat16_fat_item_free(struct fat_item* item)
{
    if(item->type == FAT_ITEM_TYPE_DIRECTORY)
    {
        fat16_free_directory(item->directory);
    }
    else if(item->type == FAT_ITEM_TYPE_FILE)
    {
        kfree(item->item);
    }

    kfree(item);
}

//Returns a directory from 
struct fat_directory* fat16_load_fat_directory(struct disk* disk, struct fat_directory_item* item)
{
    uint32_t res = 0;
    struct fat_directory* directory = 0;
    struct fat_private* fat_private = disk->fs_private;
    if(!(item->attribute & FAT_FILE_SUBDIRECTORY)) //We dont want a file
    {
        res = -EINVARG;
        goto out;
    }
    //Allocate the returned directory
    directory = kzalloc(sizeof(struct fat_directory));
    if(!directory)
    {
        res = -ENOMEM;
        goto out;
    }

    uint32_t cluster = fat16_get_first_cluster(item); //Get first cluster of the item
    uint32_t cluster_sector = fat16_cluster_to_sector(fat_private, cluster); //Change the cluster to sector
    uint32_t total_items = fat16_get_total_items_for_directory(disk, cluster_sector); //Sets the total items in the directory
    directory->total = total_items; //Assigns the value to the returned directory
    uint32_t directory_size = directory->total * sizeof(struct fat_directory_item); 
    directory->item = kzalloc(directory_size); //Asigns memory for all the items in the directory + the directory struct itself
    if(!directory->item)
    {
        res = -ENOMEM;
        goto out;
    }

    //All the data is stored to directory->item
    res = fat16_read_internal(disk, cluster, 0x00, directory_size, directory->item);
    if(res != CROSOS_ALL_OK)
    {
        goto out;
    }

out:
    if(res != CROSOS_ALL_OK)
    {
        fat16_free_directory(directory); //Error, we dont want the space for the created directory anymore
    }
    return directory;
}

//Creates a new item to be treated from the data in 'item', to avoid memory issues, since item is not allocated
struct fat_item* fat16_new_fat_item_for_directory_item(struct disk* disk, struct fat_directory_item* item)
{
    //Allocates the new item in memory
    struct fat_item* f_item = kzalloc(sizeof(struct fat_item));
    if(!f_item)
    {
        return 0;
    }

    if(item->attribute & FAT_FILE_SUBDIRECTORY) //Directory case
    {
        f_item->directory = fat16_load_fat_directory(disk, item); //Get the directory from the 'item'
        f_item->type = FAT_ITEM_TYPE_DIRECTORY;
        return f_item;
    }

    f_item->type = FAT_ITEM_TYPE_FILE;
    //Allocates a copy of the item to memory and inside the return value
    f_item->item = fat16_clone_directory_item(item, sizeof(struct fat_directory_item));
    return f_item;
}

//Return an item (directory or file) from a given 'directory', 'disk', and 'name' (from the parsing of the route)
struct fat_item* fat16_find_item_in_directory(struct disk* disk, struct fat_directory* directory, const char* name)
{
    struct fat_item* f_item = 0; //Initialize the returned item
    char tmp_filename[CROSOS_MAX_PATH]; //Create a temporary string to check coincidences on the name of the item we are looking for
    //Iterate all the items in a directory
    for(uint32_t i = 0; i < directory->total; i++)
    {
        //Get filename of the item
        fat16_get_full_relative_filename(&directory->item[i], tmp_filename, sizeof(tmp_filename));

        if(istrncmp(tmp_filename, name, sizeof(tmp_filename)) == 0) //Compare it with the name we are looking for
        {
            //Found. Create a new FAT item
            f_item = fat16_new_fat_item_for_directory_item(disk, &directory->item[i]);
        }
    }

    return f_item;
}

//Gets an item 
struct fat_item* fat16_get_directory_entry(struct disk* disk, struct path_part* path)
{
    struct fat_private* fat_private = disk->fs_private; //Get private header (filesystem info)
    struct fat_item* current_item = 0; //Initialize returned item
    struct fat_item* root_item 
            = fat16_find_item_in_directory(disk, &fat_private->root_directory, path->part); //Get the root directory item
    if(!root_item)
    {
        goto out;
    }

    struct path_part* next_part = path->next;
    current_item = root_item; //Prepares the current_item to the root_directory to start a loop
    while(next_part != 0) //The loop iterates until the next_part is 0, meaning that we reached the file, instead of all the previous directories
    {
        if(current_item->type != FAT_ITEM_TYPE_DIRECTORY) //Looking for a filesystem corruption. If the next part != 0 and the item_type we are checking is not a directory, it is a clear corruption
        {
            current_item = 0;
            break;
        }

        //Load the next directory / file to tmp_item
        struct fat_item* tmp_item = fat16_find_item_in_directory(disk, current_item->directory, next_part->part);
        //Free the memory for the already used directory
        fat16_fat_item_free(current_item);
        //Reload it with new information (next inner directory or the final file)
        current_item = tmp_item;
        next_part = next_part->next;
    }

out:
    return current_item; //Return the file
}
void* fat16_open(struct disk* disk, struct path_part* path, FILE_MODE mode) //Implements the open file in FAT16
{
    //Read only supported for now
    if(mode != FILE_MODE_READ)
    {
        return ERROR(-ERDONLY);
    }

    //Create and allocate a file_descriptor, containing an item (directory or file) and a position
    struct fat_file_descriptor* descriptor = 0;
    descriptor = kzalloc(sizeof(struct fat_file_descriptor));
    if(!descriptor)
    {
        return ERROR(-ENOMEM);
    }
    //Get the directory entry, from path and disk
    descriptor->item = fat16_get_directory_entry(disk, path);
    if(!descriptor->item)
    {
        return ERROR(-EIO);
    }
    descriptor->pos = 0; //Position hardcoded to 0, needs to be reimplemented?

    return descriptor;
}
static void fat16_free_file_descriptor(struct fat_file_descriptor* desc)
{
    fat16_fat_item_free(desc->item); //Deallocates the item struct of the private desriptor
    kfree(desc); //Deallocates the private descriptor
}
uint32_t fat16_close(void* private) //Frees the allocated spae for the descriptor used for the file
{
    fat16_free_file_descriptor((struct fat_file_descriptor*) private);
    return 0;
}

uint32_t fat16_stat(struct disk* disk, void* private, struct file_stat* stat)
{
    uint32_t res = 0;
    struct fat_file_descriptor* descriptor = (struct fat_file_descriptor*) private;
    struct fat_item* desc_item = descriptor->item;
    if(desc_item->type != FAT_ITEM_TYPE_FILE) //An item is required
    {
        res = -EINVARG;
        goto out;
    }

    struct fat_directory_item* ritem = desc_item->item;
    stat->filesize = ritem->file_size; //Gets size from item descriptor
    stat->flags = 0x00;
    if(ritem->attribute & FAT_FILE_READ_ONLY)
    {
        stat->flags |= FILE_STAT_READ_ONLY; //Sets the readonly flag if the attribute in the descriptor is set
    }

out:
    return res;

}

uint32_t fat16_read(struct disk* disk, void* descriptor, uint32_t size, uint32_t nmemb, char* out_ptr)
{  
    uint32_t res = 0;
    struct fat_file_descriptor* fat_desc = descriptor;
    struct fat_directory_item* item = fat_desc->item->item; //Gets the item descriptor
    uint32_t offset = fat_desc->pos;
    for(uint32_t i = 0; i < nmemb; i++) // A read every nmemb bytes
    {
        //Read from first cluster, offset and size
        res = fat16_read_internal(disk, fat16_get_first_cluster(item), offset, size, out_ptr);
        if(ISERR(res))
        {
            goto out;
        }
        
        out_ptr +=size; //Increment output and offset for further readings
        offset += size;
    }

    res = nmemb; //Return number of rounds for reading

out:
    return res;

}

uint32_t fat16_seek(void* private, uint32_t offset, FILE_SEEK_MODE seek_mode)
{
    uint32_t res = 0;
    struct fat_file_descriptor* desc = private;

    struct fat_item* desc_item = desc->item;
    if(desc_item->type != FAT_ITEM_TYPE_FILE) //We want to check a file, not a directory
    {
        res = -EINVARG;
        goto out;
    }

    struct fat_directory_item* item = desc_item->item; //Get the item descriptor
    if(offset >= item->file_size) //The offset cannot be higher than the file size
    {
        res = -EIO;
        goto out;
    }

    switch(seek_mode)
    {
        case SEEK_SET:
            desc->pos = offset; //Set position of the offset
            break;
        case SEEK_END:
            res = -EUNIMP;
            break;
        case SEEK_CUR:
            desc->pos +=offset; //Increment the position by the offset
            break;
        default:
            res = -EINVARG;
            break;
    }

out:
    return res;
}