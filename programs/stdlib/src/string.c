#include "string.h"


char tolower(char s1)
{
    if(s1 >= 65 && s1 <= 90) //ASCII table
    {
        s1 +=32;
    }
}
int strlen(const char* ptr)
{
    int i = 0;
    while(*ptr != 0)
    {
        i++;
        ptr++;
    }

    return i;
}

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

char* strncpy(char* dest, const char* src, int count)
{
    int i = 0;
    for(i = 0; i < count-1; i++)
    {
        if(src[i] == 0x00)
            break;

        dest[i] = src[i];
    }
    dest[i] = 0x00;
    return dest;
}

bool isdigit(char c)
{
    return c >= 48 && c <=57;
}
int tonumericdigit(char c)
{
    return c - 48;
}

int strnlen(const char* ptr, int max)
{
    int i = 0;
    for(i=0; i<max; i++)
    {
        if(ptr[i] == 0)
        {
            break;
        }
    }
    return i;
}

int strnlen_terminator(const char* str, int max, char terminator)
{
    int i = 0;
    for(i = 0; i < max; i++)
    {
        if(str[i] == '\0' || str[i] == terminator)
        {
            break;
        }
    }

    return i;
}

int istrncmp(const char* s1, const char* s2, int n)
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
int strncmp(const char* str1, const char* str2, int n)
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

char* sp = 0;
//Returns the following string chunk, based on splitting 'str' by 'delimiter'
char* strtok(char* str, const char* delimiters)
{
    int i = 0;
    int len = strlen(delimiters); //Delimiter length

    if(!str && !sp)
    {
        return 0; //No string or delimiter. Useless to keep going on
    }
    if(str && !sp)
    {
        sp = str; //sp still not set, assign it to the contents of the string
    }

    char* p_start = sp; //Get the beginning of the stored string in 'sp'
    while(1)
    {
        for(i = 0; i < len; i++)
        {
            if(*p_start == delimiters[i]) //If the current position is any of the delimiters
            {
                p_start++; //Ignore and go to next pointer position
                break;
            }
        }

        if(i == len)
        {
            sp = p_start; //If we have iterated through all delimiters and none has been found, set 'sp' to the beginning of the string
            break;
        }
    }

    if(*sp == '\0') //Reset 'sp' variable for future strtok calls
    {
        sp = 0;
        return sp;
    }

    while(*sp != '\0') //While we have not reach the end of file
    {
        for(i = 0; i < len; i++) //Check if current position contains any delimiter
        {
            if(*sp == delimiters[i]) 
            {
                *sp = '\0'; //If it does, terminate the string with a \0 and break the for loop
                break;
            }
        }

        sp++; //Increment pointer to the string
        if(i < len)
        {
            break; //Delimiter found, break while
        }
    }
    return p_start; //Return the pointer to the string with a \0 at the end (where the delimiter was found)
}