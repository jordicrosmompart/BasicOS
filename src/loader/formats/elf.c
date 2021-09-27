#include "elf.h"

//Returns the entry point (virtual address) of the program from the given header
void* elf_get_entry_ptr(struct elf_header* elf_header)
{
    return (void*) elf_header->e_entry;
}

//Returns the entry point (virtual address) of the program from the given header. uint32 formatted
uint32_t elf_get_entry(struct elf_header* elf_header)
{
    return elf_header->e_entry;
}