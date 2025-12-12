[BITS 32]

global _start
global syscall_exit
global syscall_print
global syscall_getch

extern main

section .text

_start:
    call main
    
    push eax
    call syscall_exit
    
syscall_exit:
    push ebp
    mov ebp, esp
    push ebx
    
    mov eax, 1          
    mov ebx, [ebp+8]    
    int 0x80
    
    pop ebx
    pop ebp
    ret

syscall_print:
    push ebp
    mov ebp, esp
    push ebx
    push ecx
    push edx
    
    mov ecx, [ebp+8]   
    xor edx, edx        
    
.loop:
    cmp byte [ecx+edx], 0
    je .done
    inc edx
    jmp .loop
    
.done:
    
    mov eax, 2         
    mov ebx, 1          

    int 0x80
    
    pop edx
    pop ecx
    pop ebx
    pop ebp
    ret
    
syscall_getch:
    push ebp
    mov ebp, esp
    push ebx
    push ecx
    push edx
    
    mov eax, 3         
    mov ebx, 0          
    mov ecx, 0         
    mov edx, 1          
    int 0x80

    
    pop edx
    pop ecx
    pop ebx
    pop ebp
    ret