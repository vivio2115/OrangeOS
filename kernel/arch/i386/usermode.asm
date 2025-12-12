[BITS 32]
[GLOBAL switch_to_task]
[GLOBAL switch_context]

switch_to_task:
    cli
    
    mov ecx, [esp + 4]  ; eip
    mov edx, [esp + 8]  ; esp
    mov esi, [esp + 12] ; cs
    mov edi, [esp + 16] ; ds
    
    test esi, 0x3
    jz .kernel_switch
    
    push edi       
    push edx      
    
    pushf
    pop eax
    or eax, 0x200  
    push eax       
    
    push esi        
    push ecx        
    
    mov ax, di     
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    iretd

.kernel_switch:
    mov ax, 0x10   
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    mov esp, edx   
    
    pushf
    pop eax
    or eax, 0x200 
    push eax        
    
    push esi       
    push ecx       
    
    iretd

switch_context:
    mov eax, [esp + 4] 
    mov edx, [esp + 8] 
    
    pushf
    push ebp
    push ebx
    push esi
    push edi
    
    mov [eax], esp
    mov esp, edx
    
    pop edi
    pop esi
    pop ebx
    pop ebp
    popf
    
    ret