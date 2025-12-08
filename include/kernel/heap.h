#ifndef HEAP_H
#define HEAP_H

#include <kernel/kernel.h>


struct heap_block {
    size_t size;
    bool free;
    struct heap_block* next;
};


struct heap_stats {
    size_t total_size;
    size_t used_size;
    size_t free_size;
    size_t block_count;
    size_t free_block_count;
};


bool kheap_init(void* start_addr, size_t size);
void* kmalloc(size_t size);
void kfree(void* ptr);
void kheap_get_stats(struct heap_stats* stats);


void* operator new(size_t size);
void* operator new[](size_t size);
void operator delete(void* ptr);
void operator delete[](void* ptr);
void operator delete(void* ptr, size_t size);

#endif 
