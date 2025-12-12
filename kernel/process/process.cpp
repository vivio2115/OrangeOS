#include <kernel/process.h>
#include <kernel/heap.h>
#include <kernel/paging.h>
#include <kernel/tss.h>
#include <kernel/elf.h>
#include <kernel/scheduler.h>
#include <drivers/vga.h>
#include <drivers/fat32.h>
#include <lib/memory.h>

static struct process process_table[MAX_PROCESSES];
static struct process* current_process = nullptr;
static uint32_t next_pid = 1;

extern "C" void switch_to_task(uint32_t eip, uint32_t esp, uint32_t cs, uint32_t ds);

void process_entry_trampoline() {
    struct process* current = process_get_current();
    
    vga_write("[TRAMPOLINE] Switching to PID ");
    char buf[12];
    itoa(current->pid, buf, 10);
    vga_write(buf);
    vga_write(" EIP=0x");
    itoa(current->context.eip, buf, 16);
    vga_write(buf);
    vga_write(" ESP=0x");
    itoa(current->context.esp, buf, 16);
    vga_writeln(buf);
    
    if (current->pid == 0) {
        vga_writeln("[TRAMPOLINE] -> Kernel mode");
        switch_to_task(current->context.eip, current->context.esp, 0x08, 0x10);
    } else {
        vga_writeln("[TRAMPOLINE] -> USER MODE (Ring 3)");
        switch_to_task(current->context.eip, current->context.esp, 0x1B, 0x23);
    }
}

void process_init() {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_table[i].state = PROCESS_UNUSED;
        process_table[i].pid = 0;
    }
    
    process_table[0].pid = 0;
    process_table[0].state = PROCESS_RUNNING;
    process_table[0].page_directory = paging_get_current_directory();
    process_table[0].kernel_stack = 0;
    current_process = &process_table[0];
    
    vga_writeln("[PROCESS] Process management initialized");
}

struct process* process_create(void* entry_point) {
    struct process* proc = nullptr;
    for (int i = 1; i < MAX_PROCESSES; i++) {
        if (process_table[i].state == PROCESS_UNUSED) {
            proc = &process_table[i];
            break;
        }
    }
    
    if (!proc) {
        vga_writeln("[PROCESS] Error: No free process slots!");
        return nullptr;
    }
    
    proc->pid = next_pid++;
    proc->state = PROCESS_READY;
    proc->parent = current_process;
    
    
    proc->page_directory = paging_clone_kernel_mappings();
    if (!proc->page_directory) {
        vga_writeln("[PROCESS] Error: Cannot create page directory!");
        proc->state = PROCESS_UNUSED;
        return nullptr;
    }
    
    
    proc->context.cr3 = (uint32_t)proc->page_directory;
    
    proc->kernel_stack = (uint32_t)kmalloc(4096) + 4096;
    
    
    proc->user_stack = 0xC0000000;
    
    
    
    
    for (int i = 0; i < 4; i++) {
        void* stack_frame = pfa_alloc_frame();
        if (!stack_frame) {
            vga_writeln("[PROCESS] Error: Cannot allocate stack!");
            proc->state = PROCESS_UNUSED;
            return nullptr;
        }
        
        paging_map_page_in_directory(proc->page_directory, 
                                     (void*)(proc->user_stack - (i + 1) * PAGE_SIZE),
                                     stack_frame,
                                     PTE_PRESENT | PTE_RW | PTE_USER);
    }
    
    proc->context.eip = (uint32_t)entry_point;
    
    
    
    
    
    
    
    
    proc->context.esp = proc->user_stack;
    proc->context.ebp = proc->user_stack;
    proc->context.eflags = 0x202; 
    proc->context.eax = 0;
    proc->context.ebx = 0;
    proc->context.ecx = 0;
    proc->context.edx = 0;
    proc->context.esi = 0;
    proc->context.edi = 0;
    
    
    uint32_t* stack = (uint32_t*)proc->kernel_stack;
    *--stack = (uint32_t)process_entry_trampoline; 
    *--stack = 0; 
    *--stack = 0; 
    *--stack = 0; 
    *--stack = 0; 
    *--stack = 0x202; 
    proc->saved_esp = (uint32_t)stack;
    
    vga_write("[PROCESS] Created process PID ");
    char buf[12];
    itoa(proc->pid, buf, 10);
    vga_writeln(buf);
    
    return proc;
}

void process_exit(int status) {
    if (!current_process || current_process->pid == 0) {
        return;
    }
    
    current_process->state = PROCESS_ZOMBIE;
    current_process->exit_code = status;
    
    vga_write("[PROCESS] Process ");
    char buf[12];
    itoa(current_process->pid, buf, 10);
    vga_write(buf);
    vga_write(" exited with status ");
    itoa(status, buf, 10);
    vga_writeln(buf);
    
    
    
}

struct process* process_get_current() {
    return current_process;
}

struct process* process_get_by_pid(uint32_t pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid == pid && 
            process_table[i].state != PROCESS_UNUSED) {
            return &process_table[i];
        }
    }
    return nullptr;
}

bool process_load_elf(const char* filename) {
    vga_write("[PROCESS] Loading ");
    vga_write(filename);
    vga_writeln("...");
    
    uint32_t file_size = fat32_get_file_size(filename);
    if (file_size == 0) {
        vga_writeln("[PROCESS] Error: File not found!");
        return false;
    }
    
    void* elf_data = kmalloc(file_size);
    if (!elf_data) {
        vga_writeln("[PROCESS] Error: Out of memory!");
        return false;
    }
    
    if (!fat32_read_file(filename, elf_data, file_size)) {
        vga_writeln("[PROCESS] Error: Cannot read file!");
        kfree(elf_data);
        return false;
    }
    
    if (!elf_validate(elf_data)) {
        vga_writeln("[PROCESS] Error: Invalid ELF file!");
        kfree(elf_data);
        return false;
    }
    
    uint32_t entry = elf_get_entry(elf_data);
    
    vga_write("[PROCESS] Entry point: 0x");
    char buf[12];
    itoa(entry, buf, 16);
    vga_writeln(buf);
    
    struct process* proc = process_create((void*)entry);
    if (!proc) {
        kfree(elf_data);
        return false;
    }

    if (!elf_load(elf_data, proc)) {
        vga_writeln("[PROCESS] Error: Cannot load ELF!");
        kfree(elf_data);
        return false;
    }
    
    kfree(elf_data);
    
    
    scheduler_add_process(proc);
    
    vga_writeln("[PROCESS] Process added to scheduler");
    vga_writeln("[PROCESS] Attempting to run process now...");
    
    
    tss_set_kernel_stack(proc->kernel_stack);
    
    
    paging_switch_directory(proc->page_directory);
    
    
    current_process = proc;
    proc->state = PROCESS_RUNNING;
    
    
    process_entry_trampoline();
    
    return true;
}