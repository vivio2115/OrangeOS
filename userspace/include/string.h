#ifndef _STRING_H
#define _STRING_H

typedef unsigned int size_t;
#define NULL ((void*)0)

size_t strlen(const char* str);
int strcmp(const char* s1, const char* s2);
void* memset(void* ptr, int value, size_t num);
void* memcpy(void* dest, const void* src, size_t num);
void itoa(int n, char* buffer, int base);

#endif