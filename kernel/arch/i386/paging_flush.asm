[BITS 32]

global paging_load_directory
global paging_enable
global paging_disable

paging_load_directory:
    mov eax, [esp + 4]  
    mov cr3, eax      
    ret

paging_enable:
    mov eax, cr0      
    or eax, 0x80000000  
    mov cr0, eax      
    ret

paging_disable:
    mov eax, cr0       
    and eax, 0x7FFFFFFF 
    mov cr0, eax        
    ret

