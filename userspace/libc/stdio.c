#include "../include/stdio.h"
#include "../include/string.h"


extern void syscall_print(const char* str);
extern char syscall_getch();
extern void itoa(int n, char* buffer, int base);

void putchar(char c) {
    char str[2] = {c, '\0'};
    syscall_print(str);
}

void puts(const char* str) {
    syscall_print(str);
    syscall_print("\n");
}

char getchar() {
    return syscall_getch();
}

void printf(const char* format, ...) {
    
    
    char** arg = (char**)&format;
    int c;
    char buf[32];
    
    arg++; 
    
    while ((c = *format++) != 0) {
        if (c != '%') {
            putchar(c);
        } else {
            char* p;
            c = *format++;
            if (c == 0) break;

            switch (c) {
                case 'd':
                case 'u': 
                    itoa(*((int*)arg++), buf, 10);
                    syscall_print(buf);
                    break;
                case 'x':
                    itoa(*((int*)arg++), buf, 16);
                    syscall_print("0x");
                    syscall_print(buf);
                    break;
                case 's':
                    p = *arg++;
                    if (!p) p = "(null)";
                    syscall_print(p);
                    break;
                case 'c':
                    putchar(*((int*)arg++));
                    break;
                case '%':
                    putchar('%');
                    break;
                default:
                    putchar(c);
                    break;
            }
        }
    }
}
