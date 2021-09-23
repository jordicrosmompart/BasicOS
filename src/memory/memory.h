#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include <stdint.h>

void* memset(void* ptr, uint32_t c, size_t size);
uint32_t memcmp(void* s1, void* s2, uint32_t count);
void* memcpy(void* dest, void* src, uint32_t len);
#endif