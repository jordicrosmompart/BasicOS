#ifndef GDT_H
#define GDT_H
#include <stdint.h>
struct gdt
{

    uint16_t segment;
    uint16_t base_first;
    uint8_t base;
    uint8_t access;
    uint8_t high_flags;
    uint8_t base_24_31_bits;

}__attribute__((packed));

struct gdt_structured
{
    uint32_t base;
    uint32_t limit;
    uint32_t type;
};

extern void gdt_load(struct gdt* gdt, uint32_t size);
void gdt_structured_to_gdt(struct gdt* gdt, struct gdt_structured* structured, uint32_t total_entries);
#endif