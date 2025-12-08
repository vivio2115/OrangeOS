
#include <kernel/shell.h>
#include <kernel/heap.h>
#include <kernel/editor.h>
#include <kernel/paging.h>
#include <drivers/vga.h>
#include <drivers/keyboard.h>
#include <drivers/timer.h>
#include <drivers/fat32.h>
#include <lib/memory.h>

static char input_buffer[SHELL_BUFFER_SIZE];
static int buffer_pos = 0;


void shell_init() {
    buffer_pos = 0;
    memset(input_buffer, 0, SHELL_BUFFER_SIZE);
}


void shell_prompt() {
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_write("OrangeOS");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_write("> ");
}


void cmd_help() {
    vga_writeln("Available commands:");
    vga_writeln("  help     - Show this help message");
    vga_writeln("  clear    - Clear the screen");
    vga_writeln("  echo     - Echo text to screen");
    vga_writeln("  info     - Show system information");
    vga_writeln("  testheap - Test heap allocation");
    vga_writeln("  meminfo  - Show memory information");
    vga_writeln("  time     - Show system uptime");
    vga_writeln("  reboot   - Reboot the system");
    vga_writeln("  halt     - Halt the system");
    vga_writeln("  ls       - List directory");
    vga_writeln("  cat      - Display file contents");
    vga_writeln("  load     - Load file to memory");
    vga_writeln("  vi       - Edit file (vi-like editor)");
    vga_writeln("  fat32debug - Debug FAT32 file system");
    vga_writeln("  mkdir    - Create directory");
    vga_writeln("  cd       - Change directory");
    vga_writeln("  pwd      - Print working directory");
    vga_writeln("  rm       - Delete file");
    vga_writeln("  paginginfo - Show paging statistics");
    vga_writeln("  testpaging - Test paging functionality");
}


void cmd_clear() {
    vga_clear();
}


void cmd_echo(const char* args) {
    vga_writeln(args);
}


void cmd_info() {
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_write(ORANGEOS_NAME);
    vga_write(" v");
    vga_writeln(ORANGEOS_VERSION);
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_writeln("A simple operating system written in C++ and Assembly");
    vga_writeln("Architecture: x86 (32-bit)");
    vga_writeln("Bootloader: OrangeBoot");
}


void cmd_testheap() {
    vga_writeln("Testing heap allocation...");
    
    
    void* ptr1 = kmalloc(64);
    if (ptr1 != NULL) {
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_writeln("  [OK] Allocated 64 bytes");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    } else {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_writeln("  [FAIL] Allocation failed!");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return;
    }
    
    
    void* ptr2 = kmalloc(128);
    void* ptr3 = kmalloc(256);
    if (ptr2 != NULL && ptr3 != NULL) {
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_writeln("  [OK] Multiple allocations successful");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    } else {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_writeln("  [FAIL] Multiple allocations failed!");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    }
    
    
    kfree(ptr2);
    void* ptr4 = kmalloc(128);
    if (ptr4 != NULL) {
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_writeln("  [OK] Free and reallocate successful");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    } else {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_writeln("  [FAIL] Free and reallocate failed!");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    }
    
    
    int* arr = new int[10];
    if (arr != NULL) {
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_writeln("  [OK] C++ new operator works");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        delete[] arr;
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_writeln("  [OK] C++ delete operator works");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    } else {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_writeln("  [FAIL] C++ new operator failed!");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    }
    
    
    kfree(ptr1);
    kfree(ptr3);
    kfree(ptr4);
    
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_writeln("Heap test completed!");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
}


void cmd_meminfo() {
    struct heap_stats stats;
    kheap_get_stats(&stats);
    
    vga_writeln("Memory Information:");
    vga_write("  Heap start: 0x");
    vga_writeln("200000");
    vga_write("  Total heap size: ");
    char size_str[32];
    itoa((int)(stats.total_size / 1024), size_str, 10);
    vga_write(size_str);
    vga_writeln(" KB");
    
    vga_write("  Used: ");
    itoa((int)(stats.used_size / 1024), size_str, 10);
    vga_write(size_str);
    vga_write(" KB (");
    if (stats.total_size > 0) {
        uint32_t percent = (stats.used_size * 100) / stats.total_size;
        itoa((int)percent, size_str, 10);
        vga_write(size_str);
    } else {
        vga_write("0");
    }
    vga_writeln("%)");
    
    vga_write("  Free: ");
    itoa((int)(stats.free_size / 1024), size_str, 10);
    vga_write(size_str);
    vga_write(" KB (");
    if (stats.total_size > 0) {
        uint32_t percent = (stats.free_size * 100) / stats.total_size;
        itoa((int)percent, size_str, 10);
        vga_write(size_str);
    } else {
        vga_write("0");
    }
    vga_writeln("%)");
    
    vga_write("  Total blocks: ");
    itoa((int)stats.block_count, size_str, 10);
    vga_writeln(size_str);
    
    vga_write("  Free blocks: ");
    itoa((int)stats.free_block_count, size_str, 10);
    vga_writeln(size_str);
}


void cmd_reboot() {
    vga_writeln("Rebooting system...");
    
    
    asm volatile("cli");
    
    
    
    asm volatile(
        "movb $0xFE, %%al\n\t"
        "outb %%al, $0x64"
        : : : "al", "memory"
    );
    
    
    for (volatile int i = 0; i < 1000000; i++);
    
    
    
    asm volatile(
        "movb $0x06, %%al\n\t"
        "outb %%al, $0xCF9"
        : : : "al", "memory"
    );
    
    
    
    struct {
        uint16_t limit;
        uint32_t base;
    } invalid_idt = {0, 0};
    
    asm volatile("lidt %0" : : "m" (invalid_idt));
    asm volatile("int $0x00");
    
    
    vga_writeln("Reboot failed! System halted.");
    while (1) {
        asm volatile("hlt");
    }
}


void cmd_halt() {
    vga_writeln("Halting system...");
    vga_writeln("System halted. Press Ctrl+Alt+Del to restart.");
    while (1) {
        asm volatile("hlt");
    }
}


void cmd_ls(const char* args) {
    if (args == NULL || strlen(args) == 0) {
        fat32_list_directory("/");
    } else {
        fat32_list_directory(args);
    }
}


void cmd_cat(const char* args) {
    if (args == NULL || strlen(args) == 0) {
        vga_writeln("Usage: cat <filename>");
        return;
    }
    
    
    void* buffer = kmalloc(65536);
    if (buffer == NULL) {
        vga_writeln("Error: Could not allocate memory for file");
        return;
    }
    
    if (fat32_read_file(args, buffer, 65536)) {
        
        char* file_content = (char*)buffer;
        vga_writeln(file_content);
    } else {
        vga_write("Error: Could not read file '");
        vga_write(args);
        vga_writeln("'");
    }
    
    kfree(buffer);
}


void cmd_load(const char* args) {
    if (args == NULL || strlen(args) == 0) {
        vga_writeln("Usage: load <filename>");
        return;
    }
    
    uint32_t file_size = fat32_get_file_size(args);
    if (file_size == 0) {
        vga_write("Error: File '");
        vga_write(args);
        vga_writeln("' not found or empty");
        return;
    }
    
    void* buffer = kmalloc(file_size);
    if (buffer == NULL) {
        vga_writeln("Error: Could not allocate memory for file");
        return;
    }
    
    if (fat32_read_file(args, buffer, file_size)) {
        vga_write("File '");
        vga_write(args);
        vga_write("' loaded to memory at address: ");
        
        uint32_t addr = (uint32_t)buffer;
        char hex_chars[] = "0123456789ABCDEF";
        vga_write("0x");
        bool started = false;
        for (int i = 7; i >= 0; i--) {
            uint8_t nibble = (addr >> (i * 4)) & 0xF;
            if (nibble != 0 || started || i == 0) {
                vga_putchar(hex_chars[nibble]);
                started = true;
            }
        }
        vga_writeln("");
    } else {
        vga_write("Error: Could not load file '");
        vga_write(args);
        vga_writeln("'");
    }
    
    kfree(buffer);
}


void cmd_vi(const char* args) {
    if (args == NULL || strlen(args) == 0) {
        vga_writeln("Usage: vi <filename>");
        return;
    }
    
    editor_open(args);
    editor_run();
}


void cmd_mkdir(const char* args) {
    if (args == NULL || strlen(args) == 0) {
        vga_writeln("Usage: mkdir <dirname>");
        return;
    }
    
    if (fat32_create_directory(args)) {
        vga_write("Directory '");
        vga_write(args);
        vga_writeln("' created");
    } else {
        vga_write("Error: Failed to create directory '");
        vga_write(args);
        vga_writeln("'");
    }
}


void cmd_cd(const char* args) {
    if (args == NULL || strlen(args) == 0) {
        
        if (fat32_change_directory("/")) {
            vga_writeln("Changed to root directory");
        } else {
            vga_writeln("Error: Failed to change directory");
        }
        return;
    }
    
    if (fat32_change_directory(args)) {
        vga_write("Changed to '");
        vga_write(args);
        vga_writeln("'");
    } else {
        vga_write("Error: Directory '");
        vga_write(args);
        vga_writeln("' not found");
    }
}


void cmd_pwd() {
    char path[256];
    if (fat32_get_current_directory(path, sizeof(path))) {
        vga_writeln(path);
    } else {
        vga_writeln("/");
    }
}


void cmd_rm(const char* args) {
    if (args == NULL || strlen(args) == 0) {
        vga_writeln("Usage: rm <filename>");
        return;
    }
    
    if (fat32_delete_file(args)) {
        vga_write("File '");
        vga_write(args);
        vga_writeln("' deleted");
    } else {
        vga_write("Error: Failed to delete file '");
        vga_write(args);
        vga_writeln("'");
    }
}


void cmd_paginginfo() {
    struct paging_stats stats;
    paging_get_stats(&stats);
    
    vga_writeln("Paging Information:");
    vga_write("  Total frames: ");
    char buffer[32];
    itoa((int)stats.total_frames, buffer, 10);
    vga_writeln(buffer);
    
    vga_write("  Used frames: ");
    itoa((int)stats.used_frames, buffer, 10);
    vga_write(buffer);
    vga_write(" (");
    if (stats.total_frames > 0) {
        uint32_t percent = (stats.used_frames * 100) / stats.total_frames;
        itoa((int)percent, buffer, 10);
        vga_write(buffer);
    } else {
        vga_write("0");
    }
    vga_writeln("%)");
    
    vga_write("  Free frames: ");
    itoa((int)stats.free_frames, buffer, 10);
    vga_write(buffer);
    vga_write(" (");
    if (stats.total_frames > 0) {
        uint32_t percent = (stats.free_frames * 100) / stats.total_frames;
        itoa((int)percent, buffer, 10);
        vga_write(buffer);
    } else {
        vga_write("0");
    }
    vga_writeln("%)");
    
    vga_write("  Mapped pages: ");
    itoa((int)stats.mapped_pages, buffer, 10);
    vga_writeln(buffer);
    
    vga_write("  Total memory: ");
    uint32_t total_mb = (stats.total_frames * 4) / 1024;  
    itoa((int)total_mb, buffer, 10);
    vga_write(buffer);
    vga_writeln(" MB");
}


void cmd_testpaging() {
    vga_writeln("Testing paging functionality...");
    
    
    vga_write("  [1] Allocating frame...");
    void* frame = pfa_alloc_frame();
    if (frame != NULL) {
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_write(" OK");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        vga_write(" (address: 0x");
        char hex_chars[] = "0123456789ABCDEF";
        uint32_t addr = (uint32_t)frame;
        bool started = false;
        for (int i = 7; i >= 0; i--) {
            uint8_t nibble = (addr >> (i * 4)) & 0xF;
            if (nibble != 0 || started || i == 0) {
                vga_putchar(hex_chars[nibble]);
                started = true;
            }
        }
        vga_writeln(")");
        
        
        vga_write("  [2] Mapping virtual page 0xC0000000...");
        paging_map_page((void*)0xC0000000, frame, PTE_PRESENT | PTE_RW);
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_writeln(" OK");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        
        
        vga_write("  [3] Getting physical address...");
        void* phys = paging_get_physical_address((void*)0xC0000000);
        if (phys == frame) {
            vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
            vga_writeln(" OK");
            vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        } else {
            vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
            vga_writeln(" FAILED");
            vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        }
        
        
        vga_write("  [4] Unmapping page...");
        paging_unmap_page((void*)0xC0000000);
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_writeln(" OK");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        
        
        vga_write("  [5] Freeing frame...");
        pfa_free_frame(frame);
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_writeln(" OK");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_writeln("Paging test completed successfully!");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    } else {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_writeln(" FAILED - Out of memory");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    }
}


void cmd_time() {
    uint32_t ticks = timer_get_ticks();
    uint32_t seconds = ticks / 100;  
    uint32_t minutes = seconds / 60;
    uint32_t hours = minutes / 60;
    
    seconds = seconds % 60;
    minutes = minutes % 60;
    
    char buffer[32];
    
    vga_write("System uptime: ");
    if (hours > 0) {
        itoa((int)hours, buffer, 10);
        vga_write(buffer);
        vga_write(" hours, ");
    }
    if (minutes > 0) {
        itoa((int)minutes, buffer, 10);
        vga_write(buffer);
        vga_write(" minutes, ");
    }
    itoa((int)seconds, buffer, 10);
    vga_write(buffer);
    vga_writeln(" seconds");
    
    vga_write("Total ticks: ");
    itoa((int)ticks, buffer, 10);
    vga_writeln(buffer);
}


void shell_execute(const char* command) {
    
    while (*command == ' ') command++;
    
    if (strlen(command) == 0) {
        return; 
    }

    
    char cmd[32];
    const char* args = command;
    int i = 0;
    
    
    while (*args && *args != ' ' && i < 31) {
        cmd[i++] = *args++;
    }
    cmd[i] = '\0';
    
    
    while (*args == ' ') args++;

    
    if (strcmp(cmd, "help") == 0) {
        cmd_help();
    } else if (strcmp(cmd, "clear") == 0) {
        cmd_clear();
    } else if (strcmp(cmd, "echo") == 0) {
        cmd_echo(args);
    } else if (strcmp(cmd, "info") == 0) {
        cmd_info();
    } else if (strcmp(cmd, "testheap") == 0) {
        cmd_testheap();
    } else if (strcmp(cmd, "meminfo") == 0) {
        cmd_meminfo();
    } else if (strcmp(cmd, "time") == 0) {
        cmd_time();
    } else if (strcmp(cmd, "reboot") == 0) {
        cmd_reboot();
    } else if (strcmp(cmd, "halt") == 0) {
        cmd_halt();
    } else if (strcmp(cmd, "ls") == 0) {
        cmd_ls(args);
    } else if (strcmp(cmd, "cat") == 0) {
        cmd_cat(args);
    } else if (strcmp(cmd, "load") == 0) {
        cmd_load(args);
    } else if (strcmp(cmd, "vi") == 0) {
        cmd_vi(args);
    } else if (strcmp(cmd, "fat32debug") == 0) {
        fat32_debug();
    } else if (strcmp(cmd, "mkdir") == 0) {
        cmd_mkdir(args);
    } else if (strcmp(cmd, "cd") == 0) {
        cmd_cd(args);
    } else if (strcmp(cmd, "pwd") == 0) {
        cmd_pwd();
    } else if (strcmp(cmd, "rm") == 0) {
        cmd_rm(args);
    } else if (strcmp(cmd, "paginginfo") == 0) {
        cmd_paginginfo();
    } else if (strcmp(cmd, "testpaging") == 0) {
        cmd_testpaging();
    } else {
        vga_write("Unknown command: ");
        vga_writeln(cmd);
        vga_writeln("Type 'help' for available commands");
    }
}


void shell_run() {
    shell_prompt();
    
    while (true) {
        char c = keyboard_getchar();
        
        if (c == '\n') {
            
            vga_putchar('\n');
            input_buffer[buffer_pos] = '\0';
            shell_execute(input_buffer);
            
            
            buffer_pos = 0;
            memset(input_buffer, 0, SHELL_BUFFER_SIZE);
            
            
            shell_prompt();
        } else if (c == '\b') {
            
            if (buffer_pos > 0) {
                buffer_pos--;
                input_buffer[buffer_pos] = '\0';
                vga_putchar('\b');
            }
        } else if (buffer_pos < SHELL_BUFFER_SIZE - 1) {
            
            input_buffer[buffer_pos++] = c;
            vga_putchar(c);
        }
    }
}
