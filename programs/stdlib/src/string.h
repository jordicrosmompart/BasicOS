#ifndef CROSOS_STRING_H
#define CROSOS_STRING_H

#include <stdbool.h>

int strlen(const char* ptr);
bool isdigit(char c);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, int count);
int tonumericdigit(char c);
int strnlen(const char* ptr, int max);
int strncmp(const char* str1, const char* str2, int n);
int istrncmp(const char* s1, const char* s2, int n);
int strnlen_terminator(const char* str, int max, char terminator);
char tolower(char s1);
char* strtok(char* str, const char* delimiters);

#endif