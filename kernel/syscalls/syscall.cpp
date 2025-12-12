#include <kernel/syscall.h>
#include <kernel/idt.h>
#include <kernel/process.h>
#include <kernel/scheduler.h>
#include <kernel/paging.h>
#include <kernel/tss.h>
#include <drivers/vga.h>
#include <kernel/panic.h>
#include <lib/memory.h>
#include <drivers/keyboard.h>

extern "C" void syscall_handler();
extern "C" void switch_to_task(uint32_t eip, uint32_t esp, uint32_t cs, uint32_t ds);

void syscall_init() {
    
    
    idt_set_gate(0x80, (uint32_t)syscall_handler, 0x08, 0xEE);
    
    vga_writeln("[SYSCALL] System call interface initialized (INT 0x80)");
}

extern "C" void syscall_dispatcher(struct syscall_regs* regs) {
    switch(regs->eax) {
        case SYS_EXIT:
            syscall_exit(regs->ebx);
            break;
        
        case SYS_WRITE:
            regs->eax = syscall_write(regs->ebx, (const char*)regs->ecx, regs->edx);
            break;
            
        case SYS_READ:
            regs->eax = syscall_read(regs->ebx, (char*)regs->ecx, regs->edx);
            break;
            
            
        default:
            vga_write("[SYSCALL] Unknown syscall: ");
            char buf[12];
            itoa(regs->eax, buf, 10);
            vga_writeln(buf);
            regs->eax = -1;
            break;
            
    }
}

void syscall_exit(int status) {
    vga_write("[SYSCALL] Process exited with code: ");
    char buf[12];
    itoa(status, buf, 10);
    vga_writeln(buf);
    
    struct process* current = process_get_current();
    if (current) {
        process_exit(status);
    }
    
    
    struct process* next = scheduler_get_next();
    if (next) {
        
        paging_switch_directory(next->page_directory);
        tss_set_kernel_stack(next->kernel_stack);
        next->state = PROCESS_RUNNING;
        
        if (next->pid == 0) {
            switch_to_task(next->context.eip, next->context.esp, 0x08, 0x10);
        } else {
            switch_to_task(next->context.eip, next->context.esp, 0x1B, 0x23);
        }
    } else {
        
        
        vga_writeln("[SYSCALL] No ready processes, entering idle loop");
        while (1) {
            asm volatile("sti; hlt");
        }
    }
}

int syscall_write(int fd, const char* buf, size_t count) {
    if (fd == 1 || fd == 2) {
        for (size_t i = 0; i < count; i++) {
            vga_putchar(buf[i]);
        }
        return count;
    }
    return -1;
}

int syscall_read(int fd, char* buf, size_t count) {
    if (fd == 0) { 
        
        if (buf == NULL && count == 1) {
            vga_writeln("[SYSCALL] Reading single char (direct return)");
            asm volatile("sti"); 
            char c = keyboard_getchar();
            asm volatile("cli"); 
            return (int)(unsigned char)c; 
        }
        
        
        if ((uint32_t)buf < 0x40000000 || (uint32_t)buf >= 0xC0001000) {
            vga_write("[SYSCALL] Invalid buffer address: 0x");
            char dbg[12];
            itoa((uint32_t)buf, dbg, 16);
            vga_writeln(dbg);
            return -1;
        }
        
        vga_write("[SYSCALL] Reading ");
        char dbg[12];
        itoa(count, dbg, 10);
        vga_write(dbg);
        vga_write(" bytes to buffer at 0x");
        itoa((uint32_t)buf, dbg, 16);
        vga_writeln(dbg);
        
        asm volatile("sti"); 
        for (size_t i = 0; i < count; i++) {
            char c = keyboard_getchar();
            buf[i] = c;
        }
        asm volatile("cli"); 
        return count;
    }
    return -1;
}