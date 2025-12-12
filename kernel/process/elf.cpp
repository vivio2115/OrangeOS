#include <kernel/elf.h>
#include <kernel/paging.h>
#include <drivers/vga.h>
#include <lib/memory.h>

bool elf_validate(void* elf_data) {
    elf32_ehdr_t* ehdr = (elf32_ehdr_t*)elf_data;
    
    if (ehdr->e_ident[EI_MAG0] != ELFMAG0 ||
        ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr->e_ident[EI_MAG2] != ELFMAG2 ||
        ehdr->e_ident[EI_MAG3] != ELFMAG3) {
        return false;
    }
    
    if (ehdr->e_ident[EI_CLASS] != ELFCLASS32) {
        return false;
    }
    
    if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB) {
        return false;
    }
    
    if (ehdr->e_type != ET_EXEC) {
        return false;
    }
    
    if (ehdr->e_machine != EM_386) {
        return false;
    }
    
    return true;
}

uint32_t elf_get_entry(void* elf_data) {
    elf32_ehdr_t* ehdr = (elf32_ehdr_t*)elf_data;
    return ehdr->e_entry;
}

bool elf_load(void* elf_data, struct process* proc) {
    if (!elf_validate(elf_data)) {
        return false;
    }
    
    elf32_ehdr_t* ehdr = (elf32_ehdr_t*)elf_data;
    elf32_phdr_t* phdr = (elf32_phdr_t*)((uint32_t)elf_data + ehdr->e_phoff);
    
    for (uint16_t i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            uint8_t* src = (uint8_t*)elf_data + phdr[i].p_offset;
            uint32_t vaddr = phdr[i].p_vaddr;
            uint32_t size = phdr[i].p_memsz;
            
            
            if (vaddr < 0x600000) {
                vga_writeln("[ELF] Error: Segment overlaps kernel memory!");
                return false;
            }
            
            vga_write("[ELF] Loading segment: vaddr=0x");
            char buf[12];
            itoa(vaddr, buf, 16);
            vga_write(buf);
            vga_write(" size=");
            itoa(size, buf, 10);
            vga_writeln(buf);
            
            
            uint32_t num_pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
            
            for (uint32_t page = 0; page < num_pages; page++) {
                void* physical = pfa_alloc_frame();
                if (!physical) {
                    return false;
                }
                
                
                void* virt_addr = (void*)(vaddr + page * PAGE_SIZE);
                paging_map_page_in_directory(proc->page_directory, virt_addr, physical, 
                                             PTE_PRESENT | PTE_RW | PTE_USER);
                
                
                uint32_t offset = page * PAGE_SIZE;
                uint32_t copy_size = PAGE_SIZE;
                if (offset >= phdr[i].p_filesz) {
                    
                    memset(physical, 0, PAGE_SIZE);
                } else {
                    if (offset + copy_size > phdr[i].p_filesz) {
                        copy_size = phdr[i].p_filesz - offset;
                    }
                    memcpy(physical, src + offset, copy_size);
                    
                    
                    if (copy_size < PAGE_SIZE) {
                        memset((uint8_t*)physical + copy_size, 0, PAGE_SIZE - copy_size);
                    }
                }
            }
        }
    }
    
    return true;
}