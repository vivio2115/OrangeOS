#include <kernel/kernel.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/heap.h>
#include <kernel/paging.h>
#include <kernel/version.h>
#include <drivers/vga.h>
#include <drivers/keyboard.h>
#include <drivers/timer.h>
#include <drivers/ata.h>
#include <drivers/fat32.h>
#include <drivers/speaker.h>
#include <kernel/shell.h>
#include <kernel/tss.h>
#include <kernel/syscall.h>
#include <kernel/process.h>
#include <kernel/scheduler.h>


extern "C" void kernel_main() {
    
    vga_init();

    vga_set_color(VGA_COLOR_ORANGE, VGA_COLOR_BLACK);
    vga_write_at("   ____                              ____  _____ ", 2, 6);
    vga_write_at("  / __ \\_________ _____  ____ ____  / __ \\/ ___/ ", 2, 7);
    vga_write_at(" / / / / ___/ __ `/ __ \\/ __ `/ _ \\/ / / /\\__ \\  ", 2, 8);
    vga_write_at("/ /_/ / /  / /_/ / / / / /_/ /  __/ /_/ /___/ /  ", 2, 9);
    vga_write_at("\\____/_/   \\__,_/_/ /_/\\__, /\\___/\\____//____/   ", 2, 10);
    vga_write_at("                      /____/                      ", 2, 11);

    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_write_at("       ___     ", 2, 13);
    vga_write_at(" _   _<  /     ", 2, 14);
    vga_write_at("| | / / /      ", 2, 15);
    vga_write_at("| |/ / /       ", 2, 16);
    vga_write_at("|___/_/        ", 2, 17);
    
    vga_set_color(VGA_COLOR_ORANGE, VGA_COLOR_BLACK);
    vga_write_at("       \\`*-.            ", 52, 6);
    vga_write_at("        )  _`-.         ", 52, 7);
    vga_write_at("       .  : `. .        ", 52, 8);
    vga_write_at("       : _   '  \\       ", 52, 9);
    vga_write_at("       ; *` _.   `*-._  ", 52, 10);
    vga_write_at("       `-.-'       `-.  ", 52, 11);
    vga_write_at("         ;       `    `.", 52, 12);
    vga_write_at("         :.       .     ", 52, 13);
    vga_write_at("         . \\  .   :   .-", 52, 14);
    vga_write_at("         '  `+.;  ;  '  ", 52, 15);
    vga_write_at("         :  '  |    ;   ", 52, 16);
    vga_write_at("         ; '   : :`-:   ", 52, 17);
    vga_write_at("     .*' /  .*' ; .*`- +", 52, 18);
    vga_write_at("      `*-*   `*-*  `*-*'", 52, 19);

    bool all_ok = true;
    
    all_ok &= gdt_init();
    all_ok &= idt_init();
    all_ok &= kheap_init((void*)HEAP_START, HEAP_SIZE);
    
    paging_init();
    void* page_dir = paging_get_current_directory();
    if (page_dir != NULL) {
        paging_load_directory(page_dir);
        paging_enable();
    } else {
        all_ok = false;
    }
    
    uint32_t kernel_stack = (uint32_t)kmalloc(4096) + 4096;
    tss_init(kernel_stack);
    
    syscall_init();
    
    process_init();
    scheduler_init();
    
    all_ok &= timer_init(100);
    
    asm volatile("sti");
    
    timer_sleep(2000);
    
    vga_clear();
    vga_set_cursor(0, 0);
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_writeln("=== System Initialization ===");
    vga_writeln("");
    
    vga_write("[*] Initializing GDT...");
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_writeln(" OK");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    vga_write("[*] Initializing IDT...");
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_writeln(" OK");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    vga_write("[*] Initializing heap...");
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_writeln(" OK");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    vga_write("[*] Initializing paging...");
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_writeln(" OK");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    vga_write("[*] Initializing TSS...");
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_writeln(" OK");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    vga_write("[*] Initializing system calls...");
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_writeln(" OK");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    vga_write("[*] Initializing process manager...");
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_writeln(" OK");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    vga_write("[*] Initializing scheduler...");
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_writeln(" OK");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    vga_write("[*] Initializing timer...");
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_writeln(" OK");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    vga_write("[*] Enabling interrupts...");
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_writeln(" OK");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    vga_write("[*] Initializing PC speaker...");
    Speaker::init();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_writeln(" OK");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    vga_write("[*] Initializing keyboard...");
    bool keyboard_ok = keyboard_init();
    if (keyboard_ok) {
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_writeln(" OK");
    } else {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_writeln(" FAILED");
    }
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    vga_write("[*] Initializing disk driver...");
    bool ata_ok = ata_init();
    if (ata_ok) {
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_writeln(" OK");
    } else {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_writeln(" FAILED");
    }
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    vga_write("[*] Initializing file system...");
    fat32_init();
    if (fat32_is_initialized()) {
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_writeln(" OK");
    } else {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_writeln(" FAILED");
    }
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    vga_writeln("");
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_writeln("System initialized successfully!");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_writeln("");

    Speaker::beep_startup();
    
    timer_sleep(100);
    
    vga_clear();
    vga_set_cursor(0, 0);

    // process_load_elf("init.elf");
    
    vga_set_color(VGA_COLOR_ORANGE, VGA_COLOR_BLACK);
    vga_writeln("Welcome to " OS_NAME);
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_writeln(OS_FULL_INFO);
    vga_writeln("Type 'help' for available commands.");
    vga_writeln(" ");
    shell_init();

    scheduler_start();
    
    shell_run();
    
    while (1) {
        asm volatile("hlt");
    }
}