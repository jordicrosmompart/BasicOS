#include "elfloader.h"
#include "fs/file.h"
#include "status.h"
#include <stdbool.h>
#include "memory/memory.h"
#include "memory/heap/kheap.h"
#include "string/string.h"
#include "memory/paging/paging.h"
#include "kernel.h"
#include "config.h"

//The following signature is verified to determine if the file parsed is elf formatted
const char elf_signature[] = {0x7f, 'E', 'L', 'F'};

//Compares an input buffer with the signature
static bool elf_valid_signature(void* buffer)
{
    return memcmp(buffer, (void*) elf_signature, sizeof(elf_signature)) == 0;
}

//Determines if the class (type of file, 32/64 bits, ISA, etc) is compatible with our system (32bits)
static bool elf_valid_class(struct elf_header* header)
{
    //We only support 32 bit binaries for now.
    return header->e_ident[EI_CLASS] == ELFCLASSNONE || header->e_ident[EI_CLASS] == ELFCLASS32;
}

//Determines if the enocoding of the data (little endian, big endian, etc) is compatible with out system
static bool elf_valid_encoding(struct elf_header* header)
{
    return header->e_ident[EI_DATA] == ELFDATANONE || header->e_ident[EI_DATA] == ELFDATA2LSB; //Two's complement + little endian
}

//Checks that the file is an executable + its entry address is 0x400000 (virtual address assigned by our system to load our programs)
static bool elf_is_executable(struct elf_header* header)
{
    return header->e_type == ET_EXEC && header->e_entry >= CROSOS_PROGRAM_VIRTUAL_ADDRESS;
}

//Check if the file has a program header
static bool elf_has_program_header(struct elf_header* header)
{
    return header->e_phoff != 0;
}

//Returns the physical address where the file is loaded
void* elf_memory(struct elf_file* file)
{
    return file->elf_memory;
}

//Returns the address of the header (beginning of the file)
struct elf_header* elf_header(struct elf_file* file)
{
    return file->elf_memory;
}

//Returns the address of the section's headers table
struct elf32_shdr* elf_sheader(struct elf_header* header)
{
    return (struct elf32_shdr*) ((int32_t) header + header->e_shoff);
}

//Returns the address of the program's headers table
struct elf32_phdr* elf_pheader(struct elf_header* header)
{
    if(header->e_phoff == 0)
    {
        return 0;
    }

    return (struct elf32_phdr*) ((int32_t) header + header->e_phoff);
}

//Gets a program header from the table and an index
struct elf32_phdr* elf_program_header(struct elf_header* header, int32_t index)
{
    return &elf_pheader(header)[index];
}

//Gets a section header from the table and an index
struct elf32_shdr* elf_section(struct elf_header* header, int32_t index)
{
    return &elf_sheader(header)[index];
}

//Physical address of the part described by the program header
void* elf_phdr_phys_address(struct elf_file* file, struct elf32_phdr* phdr)
{
    return elf_memory(file) + phdr->p_offset;
}

//Gets string table address address
char* elf_str_table(struct elf_header* header)
{
    return (char*) header + elf_section(header, header-> e_shstrndx)->sh_offset;
}

//Returns the virtual base address
void* elf_virtual_base(struct elf_file* file)
{
    return file->virtual_base_address;
}

//Returns the virtual end address
void* elf_virtual_end(struct elf_file* file)
{
    return file->virtual_end_address;
}

//Returns the physical base addres
void* elf_phys_base(struct elf_file* file)
{
    return file->physical_base_address;
}

//Returns the physical end addres
void* elf_phys_end(struct elf_file* file)
{
    return file->physical_end_address;
}

//Validates signature, class and ecoding
int32_t elf_validate_loaded(struct elf_header* header)
{
    return (elf_valid_signature(header) && elf_valid_class(header) && elf_valid_encoding(header) && elf_has_program_header(header) ? CROSOS_ALL_OK : -EINFORMAT);
}

//Processes the "load" program header. Calculates virtual and physical base addresses
int32_t elf_process_phdr_pt_load(struct elf_file* elf_file, struct elf32_phdr* phdr) 
{
    //If the virtual base address is not set or the found value is smaller, set it to that value
    //This is done because we may iterate through several "load" program headers, which all of them contain a v_addr. We need the lowest one
    if(elf_file->virtual_base_address >= (void*) phdr->p_vaddr || elf_file->virtual_base_address == 0x00)
    {
        elf_file->virtual_base_address = (void*) phdr->p_vaddr; //The vaddr is specified at the header

        elf_file->physical_base_address = elf_memory(elf_file)+phdr->p_offset; //Sets physical base address
    }

    uint32_t end_virtual_address = phdr->p_vaddr + phdr->p_filesz; //End address local varialbe
    //Set the virtual end address to the highest possible value of all "load" program headers
    if(elf_file->virtual_end_address <= (void*) (end_virtual_address) || elf_file->virtual_end_address == 0x00)
    {
        elf_file->virtual_end_address = (void*) end_virtual_address;
        elf_file->physical_end_address = elf_memory(elf_file) + phdr->p_offset + phdr->p_filesz; //Base address of the file + address of the program + program size
    }
    return 0;
}

//Processes a program header entry
int32_t elf_process_pheader(struct elf_file* elf_file, struct elf32_phdr* phdr)
{
    int32_t res = 0;
    switch(phdr->p_type)
    {
        case PT_LOAD: //For now, we only process the load program header. 
            res = elf_process_phdr_pt_load(elf_file, phdr);
            break;
    }
    return res;
}
int32_t elf_process_pheaders(struct elf_file* elf_file)
{
    int32_t res = 0;
    struct elf_header* header = elf_header(elf_file); //Gets the elf header
    for (int32_t i = 0; i < header->e_phnum; i++) //For every entry in the program header entry table
    {
        struct elf32_phdr* phdr = elf_program_header(header, i); //Get the program header
        res = elf_process_pheader(elf_file, phdr); //proceess it
        if(res < 0)
        {
            break;
        }
    }
    return res;
}

int32_t elf_process_loaded(struct elf_file* elf_file)
{
    int res = 0;
    struct elf_header* header = elf_header(elf_file); //Gets the elf header
    res = elf_validate_loaded(header); //Validate that the file will be able to be executed in our system
    if(res < 0)
    {
        goto out;
    }

    res = elf_process_pheaders(elf_file); //Process the headers
    if(res < 0)
    {
        goto out;
    }

out:
    return res;
}

//Loads an elf file into memory and parses its contents
int32_t elf_load(const char* filename, struct elf_file** file_out)
{
    struct elf_file* elf_file = kzalloc(sizeof(struct elf_file)); //Allocate a struct
    int32_t fd = 0;
    int res = fopen(filename, "r"); //Open the file
    if(res <= 0)
    {
        goto out;
    }

    fd = res;
    struct file_stat stat;
    res = fstat(fd, &stat); //Gets size
    if( res < 0)
    {
        goto out;
    }

    elf_file->elf_memory = kzalloc(stat.filesize); //Allocates space for the file and stores the address at 'elf_memory'
    res = fread(elf_file->elf_memory, stat.filesize, 1, fd); //Places the file in the allocated space
    if(res < 0)
    {
        goto out;
    }

    //Load the elf_file contents
    res = elf_process_loaded(elf_file);
    if(res < 0)
    {
        goto out;
    }

    *file_out = elf_file; //Returns the address of the allocated struct
out:
    fclose(fd); //Closes the filesystem handler
    return res;
}

void elf_close(struct elf_file* file)
{
    if(!file)
    {
        return;
    }
    kfree(file->elf_memory); //Free allocated space from loaded file
    kfree(file); //Free allocated space for structure
}