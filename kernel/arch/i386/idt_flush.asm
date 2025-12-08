[BITS 32]

global idt_flush

idt_flush:
    mov eax, [esp + 4] 
    lidt [eax]        
    ret

global isr0
global isr1

isr0:
    cli
    push byte 0       
    push byte 0     
    jmp isr_common_stub

isr1:
    cli
    push byte 0
    push byte 1
    jmp isr_common_stub

global isr14
isr14:
    cli
    push byte 14      
    jmp isr_common_stub

extern isr_exception_handler
isr_common_stub:
    pusha            

    mov ax, ds
    push eax         

    mov ax, 0x10    
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp        
    call isr_exception_handler
    add esp, 4      

    pop eax          
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa           
    add esp, 8       
    sti
    iret             

global irq0
extern timer_handler

irq0:
    cli
    pusha

    mov ax, ds
    push eax

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call timer_handler

    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa
    sti
    iret

global irq1
extern keyboard_handler

irq1:
    cli
    pusha

    mov ax, ds
    push eax

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call keyboard_handler

    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa
    sti
    iret

