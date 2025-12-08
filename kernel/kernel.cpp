#include <kernel/kernel.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/heap.h>
#include <kernel/paging.h>
#include <drivers/vga.h>
#include <drivers/keyboard.h>
#include <drivers/timer.h>
#include <drivers/ata.h>
#include <drivers/fat32.h>
#include <kernel/shell.h>


extern "C" void kernel_main() {
    
    vga_init();
    
    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_write(" ____         ____ _____ \n");
    vga_write(" / __ \\       / __ \\ / ____|\n");
    vga_write("| |  | |_ __ __ _ _ __  __ _  ___| |  | | (___  \n");
    vga_write("| |  | | '__/ _` | '_ \\ / _` |/ _ \\ |  | |\\___ \\ \n");
    vga_write("| |__| | | | (_| | | | | (_| |  __/ |__| |____) |\n");
    vga_write(" \\____/|_|  \\__,_|_| |_|\\__, |\\___|\\____/|_____/ \n");
    vga_write("                          __/ |                  \n");
    vga_write("                         |___/                   \n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_writeln("");
    
    
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_write("Welcome to ");
    vga_write(ORANGEOS_NAME);
    vga_write(" v");
    vga_writeln(ORANGEOS_VERSION);
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_writeln("");
    
    
    vga_write("[*] Initializing GDT...");
    gdt_init();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_writeln(" OK");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    
    vga_write("[*] Initializing IDT...");
    idt_init();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_writeln(" OK");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    
    vga_write("[*] Initializing heap...");
    kheap_init((void*)HEAP_START, HEAP_SIZE);
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_writeln(" OK");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    
    vga_write("[*] Initializing paging...");
    paging_init();
    void* page_dir = paging_get_current_directory();
    if (page_dir != NULL) {
        paging_load_directory(page_dir);
        paging_enable();
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_writeln(" OK");
    } else {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_writeln(" FAILED");
    }
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    
    vga_write("[*] Initializing timer...");
    timer_init(100);
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_writeln(" OK");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    
    vga_write("[*] Initializing keyboard...");
    keyboard_init();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_writeln(" OK");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    
    vga_write("[*] Initializing disk driver...");
    ata_init();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_writeln(" OK");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    
    vga_write("[*] Initializing file system...");
    fat32_init();
    
    if (!fat32_is_initialized()) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_writeln(" FAILED");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    } else {
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_writeln(" OK");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    }
    
    
    vga_write("[*] Enabling interrupts...");
    asm volatile("sti");
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_writeln(" OK");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    vga_writeln("");
    vga_writeln("System initialized successfully!");
    vga_writeln("Type 'help' for available commands.");
    vga_writeln("");
    
    
    shell_init();
    shell_run();
    
    
    while (1) {
        asm volatile("hlt");
    }
}