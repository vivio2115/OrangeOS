#ifndef PROCESS_H
#define PROCESS_H

#include <kernel/kernel.h>

#define MAX_PROCESSES 32

typedef enum {
    PROCESS_UNUSED = 0,
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_BLOCKED,
    PROCESS_ZOMBIE
} process_state_t;

struct cpu_context {
    uint32_t eax, ebx, ecx, edx;
    uint32_t esi, edi, ebp, esp;
    uint32_t eip;
    uint32_t eflags;
    uint32_t cr3;
} __attribute__((packed));

struct process {
    uint32_t pid;
    process_state_t state;
    
    void* page_directory;
    uint32_t kernel_stack;
    uint32_t user_stack;
    uint32_t saved_esp; 
    
    struct cpu_context context;
    
    struct process* parent;
    int exit_code;
    
    struct process* next;
};

void process_init();
struct process* process_create(void* entry_point);
void process_exit(int status);
struct process* process_get_current();
struct process* process_get_by_pid(uint32_t pid);
bool process_load_elf(const char* filename);

#endif