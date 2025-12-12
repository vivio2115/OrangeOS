#include <kernel/scheduler.h>
#include <kernel/paging.h>
#include <kernel/tss.h>
#include <drivers/vga.h>

extern "C" void switch_to_task(uint32_t eip, uint32_t esp, uint32_t cs, uint32_t ds);
extern "C" void switch_context(uint32_t* old_esp_ptr, uint32_t new_esp);

static struct process* ready_queue_head = nullptr;
static struct process* ready_queue_tail = nullptr;
static struct process* current_running = nullptr;
static bool scheduling_enabled = false;

void scheduler_init() {
    ready_queue_head = nullptr;
    ready_queue_tail = nullptr;
    vga_writeln("[SCHEDULER] Round-robin scheduler initialized");
}

void scheduler_add_process(struct process* proc) {
    if (!proc) return;
    
    proc->next = nullptr;
    
    if (!ready_queue_head) {
        ready_queue_head = proc;
        ready_queue_tail = proc;
    } else {
        ready_queue_tail->next = proc;
        ready_queue_tail = proc;
    }
    
    proc->state = PROCESS_READY;
}

struct process* scheduler_get_next() {
    if (!ready_queue_head) {
        return nullptr;
    }
    
    struct process* next = ready_queue_head;
    ready_queue_head = next->next;
    
    if (!ready_queue_head) {
        ready_queue_tail = nullptr;
    }
    
    return next;
}

static void context_switch(struct process* next) {
    if (!next || next == current_running) {
        return;
    }
    
    struct process* prev = current_running;
    current_running = next;
    
    
    paging_switch_directory(next->page_directory);
    
    
    tss_set_kernel_stack(next->kernel_stack);
    
    
    next->state = PROCESS_RUNNING;
    
    
    switch_context(&prev->saved_esp, next->saved_esp);
}

void scheduler_schedule() {
    if (!scheduling_enabled) {
        return;
    }
    
    
    if (current_running && current_running->state == PROCESS_RUNNING) {
        current_running->state = PROCESS_READY;
        scheduler_add_process(current_running);
    }
    
    
    struct process* next = scheduler_get_next();
    
    if (next) {
        
        context_switch(next);
    } else if (current_running && current_running->state == PROCESS_RUNNING) {
        
        
        context_switch(current_running);
    }
}

void scheduler_start() {
    scheduling_enabled = true;
    vga_writeln("[SCHEDULER] Multitasking enabled");
}