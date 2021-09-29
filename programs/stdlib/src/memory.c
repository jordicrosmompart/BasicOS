#include "memory.h"

//Allocates the 'c' parameter on the values pointed by 'ptr', and the pointer increments 'size' times
void* memset(void* ptr, int c, size_t size)
{
    char* c_ptr = (char*) ptr;
    for(int i = 0; i < size; i++)
    {
        c_ptr[i] = (char) c;
    }
    return ptr;
}

//Compares the contents of two pointers for 'count' positions
int memcmp(void* s1, void* s2, int count)
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

//Copies 'len' address contents from 'src' to 'dest'
void* memcpy(void* dest, void* src, int len)
{
    char *d = dest;
    char* s = src;
    while(len--)
    {
        *d++ = *s++;
    }
    return dest;
}