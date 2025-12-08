[BITS 16]          
[ORG 0x7C00]       

start:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00    
    mov [BOOT_DRIVE], dl

    mov si, msg_boot
    call print_string

    mov bx, 0x7E00      
    mov dh, 10         
    mov dl, [BOOT_DRIVE]
    mov si, 1          
    call disk_load_lba
    jc .load_error   

    jmp 0x0000:0x7E00

.load_error:
    mov si, msg_load_error
    call print_string
    jmp $
    
    jmp $

print_string:
    pusha
    mov ah, 0x0E       
.loop:
    lodsb           
    cmp al, 0
    je .done
    int 0x10           
    jmp .loop
.done:
    popa
    ret

disk_load_lba:
    pusha
    push dx         
    push si          
    push bx            
    
    mov ah, 0x41       
    mov bx, 0x55AA
    int 0x13
    jc .try_chs        
    cmp bx, 0xAA55

    pop bx            
    pop si            
    pop dx            
    push dx          
    push si            
    push bx            
    
    push ds
    push ax
    mov ax, 0
    mov ds, ax       

    mov byte [dap_size], 0x10     
    mov byte [dap_reserved1], 0   
    mov byte [dap_count], dh     
    mov byte [dap_reserved2], 0   
    mov word [dap_offset], bx    
    mov word [dap_segment], 0     
    mov word [dap_lba_low], si   
    mov word [dap_lba_low+2], 0   
    mov word [dap_lba_high], 0    
    mov word [dap_lba_high+2], 0  
    
    mov si, dap_packet 
    mov ah, 0x42      
    mov dl, [BOOT_DRIVE]; Numer dysku (musi być w DL)
    int 0x13
    
    pop ax
    pop ds
    
    jnc .lba_success   
    jmp .try_chs
    
.lba_success:
    pop bx             
    pop si       
    pop dx           
    popa
    ret

.try_chs:
    pop bx            
    pop si           
    pop dx            
    
    xor ax, ax
    mov es, ax
    mov ah, 0x02      
    mov al, dh       
    mov ch, 0x00     
    mov cl, 0x02       
    mov dh, 0x00       
    mov dl, [BOOT_DRIVE]; Numer dysku
    push ax          

    int 0x13          
    pop cx              
    jc .disk_error     

    cmp al, cl          
    jne .disk_error
    
    popa
    ret

.disk_error:
    mov si, msg_disk_error
    call print_string
    jmp $         

msg_boot:           db 'OrangeBoot Stage 1 loading...', 0x0D, 0x0A, 0
msg_disk_error:     db 'Disk read error!', 0x0D, 0x0A, 0
msg_load_error:     db 'Failed to load Stage 2!', 0x0D, 0x0A, 0

BOOT_DRIVE:         db 0   

dap_packet:
dap_size:           db 0x10    
dap_reserved1:      db 0    
dap_count:          db 0      
dap_reserved2:      db 0    
dap_offset:         dw 0    
dap_segment:        dw 0       
dap_lba_low:        dd 0      
dap_lba_high:       dd 0      

times 510-($-$$) db 0
dw 0xAA55            
