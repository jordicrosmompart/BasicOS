#ifndef STRING_H
#define STRING_H
#include <stdint.h>
#include <stdbool.h>

uint32_t strlen(const char* ptr);
bool isdigit(char c);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, int32_t count);
uint32_t tonumericdigit(char c);
uint32_t strnlen(const char* ptr, uint32_t max);
uint32_t strncmp(const char* str1, const char* str2, uint32_t n);
uint32_t istrncmp(const char* s1, const char* s2, uint32_t n);
uint32_t strnlen_terminator(const char* str, uint32_t max, char terminator);
char tolower(char s1);


#endif