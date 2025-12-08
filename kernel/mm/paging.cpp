
#include <kernel/paging.h>
#include <kernel/heap.h>
#include <lib/memory.h>


#define MAX_FRAMES 8192  
#define FRAME_BITMAP_SIZE (MAX_FRAMES / 8)  

static uint8_t frame_bitmap[FRAME_BITMAP_SIZE];
static uint32_t total_frames = MAX_FRAMES;
static uint32_t used_frames = 0;
static uint32_t mapped_pages = 0;


static struct page_directory_entry* current_page_directory = NULL;


static uint32_t addr_to_frame(void* addr) {
    return ((uint32_t)addr) / PAGE_SIZE;
}


static void* frame_to_addr(uint32_t frame) {
    return (void*)(frame * PAGE_SIZE);
}


static void set_frame_bit(uint32_t frame) {
    if (frame >= MAX_FRAMES) return;
    frame_bitmap[frame / 8] |= (1 << (frame % 8));
}


static void clear_frame_bit(uint32_t frame) {
    if (frame >= MAX_FRAMES) return;
    frame_bitmap[frame / 8] &= ~(1 << (frame % 8));
}


static bool is_frame_used(uint32_t frame) {
    if (frame >= MAX_FRAMES) return true;
    return (frame_bitmap[frame / 8] & (1 << (frame % 8))) != 0;
}


void pfa_init() {
    
    memset(frame_bitmap, 0, FRAME_BITMAP_SIZE);
    used_frames = 0;
    
    
    
    pfa_mark_used(0, 0x100);  
}


void pfa_mark_used(uint32_t frame_start, uint32_t frame_count) {
    for (uint32_t i = 0; i < frame_count && (frame_start + i) < MAX_FRAMES; i++) {
        if (!is_frame_used(frame_start + i)) {
            set_frame_bit(frame_start + i);
            used_frames++;
        }
    }
}


void* pfa_alloc_frame() {
    for (uint32_t i = 0; i < MAX_FRAMES; i++) {
        if (!is_frame_used(i)) {
            set_frame_bit(i);
            used_frames++;
            return frame_to_addr(i);
        }
    }
    return NULL;  
}


void pfa_free_frame(void* frame) {
    if (frame == NULL) return;
    
    uint32_t frame_num = addr_to_frame(frame);
    if (frame_num >= MAX_FRAMES) return;
    
    if (is_frame_used(frame_num)) {
        clear_frame_bit(frame_num);
        used_frames--;
    }
}


uint32_t pfa_get_free_count() {
    return total_frames - used_frames;
}


static struct page_table_entry* get_page_table_entry(void* virtual_addr, bool create) {
    if (current_page_directory == NULL) return NULL;
    
    uint32_t addr = (uint32_t)virtual_addr;
    uint32_t page_dir_index = (addr >> 22) & 0x3FF;  
    uint32_t page_table_index = (addr >> 12) & 0x3FF;  
    
    struct page_directory_entry* pde = &current_page_directory[page_dir_index];
    
    
    if (!(pde->present)) {
        if (!create) return NULL;
        
        
        void* page_table_frame = pfa_alloc_frame();
        if (page_table_frame == NULL) return NULL;
        
        
        memset(page_table_frame, 0, PAGE_SIZE);
        
        
        pde->present = 1;
        pde->rw = 1;
        pde->user = 0;  
        pde->frame = addr_to_frame(page_table_frame);
    }
    
    
    struct page_table_entry* page_table = (struct page_table_entry*)frame_to_addr(pde->frame);
    
    
    return &page_table[page_table_index];
}


void paging_map_page(void* virtual_addr, void* physical_addr, uint32_t flags) {
    struct page_table_entry* pte = get_page_table_entry(virtual_addr, true);
    if (pte == NULL) return;
    
    
    if (pte->present) {
        
        uint32_t old_frame = pte->frame;
        uint32_t new_frame = addr_to_frame(physical_addr);
        if (old_frame != new_frame) {
            pfa_free_frame(frame_to_addr(old_frame));
        }
        mapped_pages--;
    }
    
    
    pte->present = (flags & PTE_PRESENT) ? 1 : 0;
    pte->rw = (flags & PTE_RW) ? 1 : 0;
    pte->user = (flags & PTE_USER) ? 1 : 0;
    pte->frame = addr_to_frame(physical_addr);
    
    
    uint32_t frame = addr_to_frame(physical_addr);
    if (!is_frame_used(frame)) {
        set_frame_bit(frame);
        used_frames++;
    }
    
    mapped_pages++;
}


void paging_unmap_page(void* virtual_addr) {
    struct page_table_entry* pte = get_page_table_entry(virtual_addr, false);
    if (pte == NULL || !pte->present) return;
    
    
    void* physical_frame = frame_to_addr(pte->frame);
    pfa_free_frame(physical_frame);
    
    
    pte->present = 0;
    pte->frame = 0;
    
    mapped_pages--;
}


void* paging_get_physical_address(void* virtual_addr) {
    struct page_table_entry* pte = get_page_table_entry(virtual_addr, false);
    if (pte == NULL || !pte->present) return NULL;
    
    uint32_t offset = ((uint32_t)virtual_addr) & 0xFFF;  
    return (void*)((pte->frame * PAGE_SIZE) + offset);
}


void* paging_create_page_directory() {
    
    void* pd_frame = pfa_alloc_frame();
    if (pd_frame == NULL) return NULL;
    
    
    struct page_directory_entry* pd = (struct page_directory_entry*)pd_frame;
    memset(pd, 0, PAGE_SIZE);
    
    return pd;
}


void paging_identity_map(void* addr, uint32_t size, uint32_t flags) {
    uint32_t start_addr = (uint32_t)addr;
    uint32_t end_addr = start_addr + size;
    
    
    start_addr = start_addr & ~0xFFF;  
    end_addr = (end_addr + 0xFFF) & ~0xFFF;  
    
    
    for (uint32_t addr = start_addr; addr < end_addr; addr += PAGE_SIZE) {
        paging_map_page((void*)addr, (void*)addr, flags);
    }
}


void paging_init() {
    
    pfa_init();
    
    
    uint32_t heap_start_frame = addr_to_frame((void*)0x200000);
    uint32_t heap_size_frames = 0x400000 / PAGE_SIZE;  
    pfa_mark_used(heap_start_frame, heap_size_frames);
    
    
    current_page_directory = (struct page_directory_entry*)paging_create_page_directory();
    if (current_page_directory == NULL) {
        
        return;
    }
    
    
    
    paging_identity_map((void*)0x0, 0x400000, PTE_PRESENT | PTE_RW);
    
    
    paging_identity_map((void*)0x200000, 0x400000, PTE_PRESENT | PTE_RW);
}


void* paging_get_current_directory() {
    return current_page_directory;
}


void paging_get_stats(struct paging_stats* stats) {
    if (stats == NULL) return;
    
    stats->total_frames = total_frames;
    stats->used_frames = used_frames;
    stats->free_frames = pfa_get_free_count();
    stats->mapped_pages = mapped_pages;
}

