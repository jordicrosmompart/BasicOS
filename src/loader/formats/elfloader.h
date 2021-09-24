#ifndef ELF_LOADER_H
#define ELF_LOADER_H

#include <stdint.h>
#include <stddef.h>

#include "elf.h"
#include "config.h"

struct elf_file
{
    char filename[CROSOS_MAX_PATH];

    int in_memory_size;

    //Pointer to the physical address where the elf file is loaded at
    void* elf_memory;

    //Virtual base address of the binary
    void* virtual_base_address;

    //Ending virtual address
    void* virtual_end_address;

    //Physical base address
    void* physical_base_address; 

    //Physical end address of the binary
    void* physical_end_address;   
};

int32_t elf_load(const char* filename, struct elf_file** file_out);
void elf_close(struct elf_file* file);
void* elf_virtual_base(struct elf_file* file);
void* elf_virtual_end(struct elf_file* file);
void* elf_phys_base(struct elf_file* file);
void* elf_phys_end(struct elf_file* file);
struct elf_header* elf_header(struct elf_file* file);
struct elf32_shdr* elf_sheader(struct elf_header* header);
struct elf32_phdr* elf_pheader(struct elf_header* header);
void* elf_memory(struct elf_file* file);
struct elf32_phdr* elf_program_header(struct elf_header* header, int32_t index);
struct elf32_shdr* elf_section(struct elf_header* header, int32_t index);
void* elf_phdr_phys_address(struct elf_file* file, struct elf32_phdr* phdr);

#endif