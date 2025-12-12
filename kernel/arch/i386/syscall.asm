[BITS 32]
[GLOBAL syscall_handler]
[EXTERN syscall_dispatcher]

syscall_handler:
    push 0        
    push 0x80     

    pusha         
    
    push ds
    push es
    push fs
    push gs
    
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    push esp
    call syscall_dispatcher
    add esp, 4
    
    pop gs
    pop fs
    pop es
    pop ds
    
    popa
    
    add esp, 8     
    
    iretd