#include "string.h"

//Turns the string into lower caps
char tolower(char s1)
{
    if(s1 >= 65 && s1 <= 90) //ASCII table
    {
        s1 +=32;
    }
}

//Counts the length of a string
uint32_t strlen(const char* ptr)
{
    uint32_t i = 0;
    while(*ptr != 0)
    {
        i++;
        ptr++;
    }

    return i;
}

//Copies strings
char* strcpy(char* dest, const char* src)
{
    char* res = dest;
    while(*src != 0)
    {
        *dest = *src;
        src +=1;
        dest +=1;
    }
    *dest = 0x00;

    return res;
}

//Copies strings for a given length
char* strncpy(char* dest, const char* src, int32_t count)
{
    int32_t i = 0;
    for(i = 0; i < count-1; i++)
    {
        if(src[i] == 0x00)
            break;

        dest[i] = src[i];
    }
    dest[i] = 0x00;
    return dest;
}

//Checks if it is a numeric ASCII char
bool isdigit(char c)
{
    return c >= 48 && c <=57;
}

//ASCII char to number
uint32_t tonumericdigit(char c)
{
    return c - 48;
}

//Counts the string length
uint32_t strnlen(const char* ptr, uint32_t max)
{
    uint32_t i = 0;
    for(i=0; i<max; i++)
    {
        if(ptr[i] == 0)
        {
            break;
        }
    }
    return i;
}

//Counts a string length for a given terminator
uint32_t strnlen_terminator(const char* str, uint32_t max, char terminator)
{
    uint32_t i = 0;
    for(i = 0; i < max; i++)
    {
        if(str[i] == '\0' || str[i] == terminator)
        {
            break;
        }
    }

    return i;
}

//Compares strings non caps aware
uint32_t istrncmp(const char* s1, const char* s2, uint32_t n)
{
    unsigned char u1, u2;
    while(n-- > 0)
    {
        u1 = (unsigned char)*s1++;
        u2 = (unsigned char)*s2++;
        if(u1 != u2 && tolower(u1) != tolower(u2))
        {
            return u1-u2;
        }
        if(u1 == '\0')
        {
            return 0;
        }
    }
}

//Compares strings for a given size
uint32_t strncmp(const char* str1, const char* str2, uint32_t n)
{
    unsigned char u1, u2;
    while(n-- > 0)
    {
        u1 = (unsigned char)*str1++;
        u2 = (unsigned char)*str2++;
        if(u1 != u2)
        {
            return u1-u2;
        }
        if(u1=='\0')
        {
            return 0;
        }
    }
}