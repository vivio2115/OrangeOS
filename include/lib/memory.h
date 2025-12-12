
#ifndef MEMORY_H
#define MEMORY_H

#include <kernel/kernel.h>


void* memset(void* dest, int val, size_t count);
void* memcpy(void* dest, const void* src, size_t count);
int memcmp(const void* s1, const void* s2, size_t n);


size_t strlen(const char* str);
int strcmp(const char* s1, const char* s2);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t n);
char* itoa(int value, char* str, int base);
int atoi(const char* str);

#endif 
