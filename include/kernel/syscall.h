#ifndef SYSCALL_H
#define SYSCALL_H

#include <kernel/kernel.h>

#define SYS_EXIT    1
#define SYS_WRITE   2
#define SYS_READ    3
#define SYS_OPEN    4
#define SYS_CLOSE   5

struct syscall_regs {
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
} __attribute__((packed));

void syscall_init();
extern "C" void syscall_dispatcher(struct syscall_regs* regs);

void syscall_exit(int status);
int syscall_write(int fd, const char* buf, size_t count);
int syscall_read(int fd, char* buf, size_t count);

#endif