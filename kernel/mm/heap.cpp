#include <kernel/heap.h>
#include <lib/memory.h>

static struct heap_block* heap_start = NULL;
static size_t heap_total_size = 0;

bool kheap_init(void* start_addr, size_t size) {
    if (start_addr == NULL || size < sizeof(struct heap_block) * 2) {
        return false;
    }
    
    heap_start = (struct heap_block*)start_addr;
    heap_start->size = size - sizeof(struct heap_block);
    heap_start->free = true;
    heap_start->next = NULL;
    heap_total_size = size;
    
    return true;
}

void* kmalloc(size_t size) {
    struct heap_block* current = heap_start;
    
    while (current != NULL) {
        if (current->free && current->size >= size) {
            
            if (current->size > size + sizeof(struct heap_block)) {
                
                struct heap_block* new_block = (struct heap_block*)((uint8_t*)current + sizeof(struct heap_block) + size);
                new_block->size = current->size - size - sizeof(struct heap_block);
                new_block->free = true;
                new_block->next = current->next;
                
                current->size = size;
                current->next = new_block;
            }
            
            current->free = false;
            return (void*)((uint8_t*)current + sizeof(struct heap_block));
        }
        current = current->next;
    }
    
    return NULL; 
}

void kfree(void* ptr) {
    if (ptr == NULL) return;
    
    struct heap_block* block = (struct heap_block*)((uint8_t*)ptr - sizeof(struct heap_block));
    block->free = true;
    
    
    if (block->next != NULL && block->next->free) {
        block->size += block->next->size + sizeof(struct heap_block);
        block->next = block->next->next;
    }
    
    
    struct heap_block* current = heap_start;
    while (current != NULL && current->next != block) {
        current = current->next;
    }
    
    if (current != NULL && current->free) {
        current->size += block->size + sizeof(struct heap_block);
        current->next = block->next;
    }
}


void* operator new(size_t size) {
    return kmalloc(size);
}

void* operator new[](size_t size) {
    return kmalloc(size);
}

void operator delete(void* ptr) {
    kfree(ptr);
}

void operator delete[](void* ptr) {
    kfree(ptr);
}

void operator delete[](void* ptr, size_t size) {
    (void)size;
    kfree(ptr);
}

void operator delete(void* ptr, size_t size) {
    (void)size;
    kfree(ptr);
}


void kheap_get_stats(struct heap_stats* stats) {
    if (stats == NULL) {
        return;
    }
    
    stats->total_size = heap_total_size;
    stats->used_size = 0;
    stats->free_size = 0;
    stats->block_count = 0;
    stats->free_block_count = 0;
    
    struct heap_block* current = heap_start;
    while (current != NULL) {
        stats->block_count++;
        if (current->free) {
            stats->free_size += current->size;
            stats->free_block_count++;
        } else {
            stats->used_size += current->size;
        }
        current = current->next;
    }
    
    
    size_t overhead = stats->block_count * sizeof(struct heap_block);
    stats->used_size += overhead;
    if (stats->used_size > stats->total_size) {
        stats->used_size = stats->total_size;
    }
    stats->free_size = stats->total_size - stats->used_size;
}
