[BITS 32]
[EXTERN kernel_main]
[EXTERN fat32_partition_offset]
global _start

_start:
    mov ebp, esp
    
    mov [fat32_partition_offset], eax
    
    call kernel_main
    
    jmp $
