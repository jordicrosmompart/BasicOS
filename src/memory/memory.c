#include "memory.h"

//Allocates the 'c' parameter on the values pointed by 'ptr', and the pointer increments 'size' times
void* memset(void* ptr, uint32_t c, size_t size)
{
    char* c_ptr = (char*) ptr;
    for(int i = 0; i < size; i++)
    {
        c_ptr[i] = (char) c;
    }
    return ptr;
}

uint32_t memcmp(void* s1, void* s2, uint32_t count)
{
    char* c1 = s1;
    char* c2 = s2;
    while(count-- > 0)
    {
        if(*c1++ != *c2++)
        {
            return c1[-1] < c2[-1] ? -1:1;
        }
    }

    return 0;
}

void* memcpy(void* dest, void* src, uint32_t len)
{
    char *d = dest;
    char* s = src;
    while(len--)
    {
        *d++ = *s++;
    }
    return dest;
}