#include "../include/string.h"

size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len])
        len++;
    return len;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

void* memcpy(void* dest, const void* src, size_t num) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    while (num--)
        *d++ = *s++;
    return dest;
}

void* memset(void* ptr, int value, size_t num) {
    unsigned char* p = (unsigned char*)ptr;
    while (num--)
        *p++ = (unsigned char)value;
    return ptr;
}

void itoa(int n, char* buffer, int base) {
    int i = 0;
    int isNegative = 0;
  
    if (n == 0) {
        buffer[i++] = '0';
        buffer[i] = '\0';
        return;
    }
  
    if (n < 0 && base == 10) {
        isNegative = 1;
        n = -n;
    }
  
    while (n != 0) {
        int rem = n % base;
        buffer[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        n = n / base;
    }
  
    if (isNegative)
        buffer[i++] = '-';
  
    buffer[i] = '\0';

    int start = 0;
    int end = i - 1;
    while (start < end) {
        char temp = buffer[start];
        buffer[start] = buffer[end];
        buffer[end] = temp;
        start++;
        end--;
    }
}