[BITS 16]
[ORG 0x7E00]

stage2_start:
    mov [BOOT_DRIVE], dl

    mov si, msg_stage2
    call print_string
    
    call boot_beep

    call find_fat32_partition

    call enable_a20

    call load_kernel

    mov eax, [PARTITION_OFFSET]
    push eax
    
    mov si, msg_protected
    call print_string
    
    cli
    lgdt [gdt_descriptor]
    
    mov ecx, cr0
    or ecx, 0x1
    mov cr0, ecx

    jmp CODE_SEG:protected_mode_start

enable_a20:
    pusha
    mov si, msg_a20
    call print_string

    call check_a20
    cmp ax, 1
    je .a20_done

    mov ax, 0x2401
    int 0x15
    call check_a20
    cmp ax, 1
    je .a20_done

    call enable_a20_kbc
    call check_a20
    cmp ax, 1
    je .a20_done

    in al, 0x92
    or al, 2
    out 0x92, al
    call check_a20
    cmp ax, 1
    je .a20_done

    mov si, msg_a20_failed
    call print_string
    jmp $

.a20_done:
    popa
    ret

check_a20:
    pushf
    push ds
    push es
    push di
    push si

    cli
    xor ax, ax
    mov es, ax
    mov di, 0x7dfe

    mov ax, 0xffff
    mov ds, ax
    mov si, 0x7e0e

    mov al, byte [es:di]
    push ax
    mov al, byte [ds:si]
    push ax

    mov byte [es:di], 0x00
    mov byte [ds:si], 0xFF

    cmp byte [es:di], 0xFF

    pop ax
    mov byte [ds:si], al
    pop ax
    mov byte [es:di], al

    mov ax, 0
    je .a20_disabled
    mov ax, 1

.a20_disabled:
    pop si
    pop di
    pop es
    pop ds
    popf
    ret

enable_a20_kbc:
    cli
    call .wait_input
    mov al, 0xAD
    out 0x64, al

    call .wait_input
    mov al, 0xD0
    out 0x64, al

    call .wait_output
    in al, 0x60
    push ax

    call .wait_input
    mov al, 0xD1
    out 0x64, al

    call .wait_input
    pop ax
    or al, 2
    out 0x60, al

    call .wait_input
    mov al, 0xAE
    out 0x64, al

    call .wait_input
    ret

.wait_input:
    in al, 0x64
    test al, 2
    jnz .wait_input
    ret

.wait_output:
    in al, 0x64
    test al, 1
    jz .wait_output
    ret

find_fat32_partition:
    pusha
    
    mov si, mbr_buffer
    mov bx, si
    mov dh, 1
    mov dl, [BOOT_DRIVE]
    mov si, 0
    call disk_load_lba
    
    cmp word [mbr_buffer + 510], 0xAA55
    jne .no_partition
    
    mov si, mbr_buffer + 446
    mov cx, 4
    
.search_loop:
    mov al, [si + 4]
    cmp al, 0x0B
    je .found_partition
    cmp al, 0x0C
    je .found_partition
    
    add si, 16
    loop .search_loop
    
    mov dword [PARTITION_OFFSET], 0
    popa
    ret

.found_partition:
    mov eax, [si + 8]
    mov [PARTITION_OFFSET], eax
    popa
    ret

.no_partition:
    mov dword [PARTITION_OFFSET], 0
    popa
    ret

disk_load_lba:
    pusha
    push dx
    push si
    push bx
    
    cli            
    mov ah, 0x41
    mov bx, 0x55AA
    int 0x13
    jc .try_chs
    cmp bx, 0xAA55
    jne .try_chs
    
    pop bx
    pop si
    pop dx
    push dx
    push si
    push bx
    
    mov byte [dap_size], 0x10
    mov byte [dap_reserved1], 0
    mov byte [dap_count], dh
    mov byte [dap_reserved2], 0
    mov word [dap_offset], bx
    push es
    pop ax
    mov word [dap_segment], ax
    mov word [dap_lba_low], si
    mov word [dap_lba_low+2], 0
    mov word [dap_lba_high], 0
    mov word [dap_lba_high+2], 0
    
    mov si, dap_packet
    mov ah, 0x42
    mov dl, [BOOT_DRIVE]
    int 0x13
    
    jnc .lba_success
    jmp .try_chs
    
.lba_success:
    pop bx
    pop si
    pop dx
    popa
    clc
    sti                
    ret

.try_chs:
    pop bx
    pop si
    pop dx
    
    push ax
    mov ax, es
    cmp ax, 0
    jne .es_ok
    xor ax, ax
    mov es, ax
.es_ok:
    pop ax
    
    mov ah, 0x02
    mov al, dh
    push ax
    mov ch, 0x00
    mov cl, 0x02
    mov dh, 0x00
    mov dl, [BOOT_DRIVE]
    
    int 0x13
    pop cx
    jc .disk_error
    
    cmp al, cl
    jne .disk_error
    
    popa
    clc
    sti              
    ret

.disk_error:
    sti               
    mov si, msg_disk_error
    call print_string
    jmp $

load_kernel:
    pusha
    mov si, msg_kernel_load
    call print_string

    mov bx, 0x1000
    mov es, bx
    mov bx, 0x0000

    mov dh, 100        
    mov dl, [BOOT_DRIVE]
    mov si, 12      
    
    call disk_load_lba
    jc .kernel_error

    mov bx, es
    add bx, 0xC80     
    mov es, bx
    mov bx, 0x0000

    mov dh, 100      
    mov dl, [BOOT_DRIVE]
    mov si, 112        
    
    call disk_load_lba
    jc .kernel_error

    popa
    ret

.kernel_error:
    mov si, msg_kernel_error
    call print_string
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

simple_delay:
    pusha
    mov cx, 0xFFFF
.delay_outer:
    push cx
    mov cx, 0x00FF
.delay_inner:
    nop
    nop
    loop .delay_inner
    pop cx
    loop .delay_outer
    popa
    ret

boot_beep:
    pusha
    
    in al, 0x61
    or al, 0x03
    out 0x61, al
    
    mov al, 0xB6
    out 0x43, al
    mov ax, 1193180 / 800  ; ~1491
    out 0x42, al
    mov al, ah
    out 0x42, al

    call simple_delay
    
    in al, 0x61
    and al, 0xFC
    out 0x61, al
    
    popa
    ret

gdt_start:
    dq 0x0

gdt_code:
    dw 0xFFFF
    dw 0x0
    db 0x0
    db 10011010b
    db 11001111b
    db 0x0

gdt_data:
    dw 0xFFFF
    dw 0x0
    db 0x0
    db 10010010b
    db 11001111b
    db 0x0

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

msg_stage2:         db 'OrangeBoot Stage 2 started', 0x0D, 0x0A, 0
msg_a20:            db 'Enabling A20 line...', 0x0D, 0x0A, 0
msg_a20_failed:     db 'Failed to enable A20!', 0x0D, 0x0A, 0
msg_kernel_load:    db 'Loading kernel...', 0x0D, 0x0A, 0
msg_kernel_error:   db 'Kernel load error!', 0x0D, 0x0A, 0
msg_protected:      db 'Entering protected mode...', 0x0D, 0x0A, 0
msg_disk_error:     db 'Disk read error!', 0x0D, 0x0A, 0

BOOT_DRIVE:         db 0
PARTITION_OFFSET:   dd 0

dap_packet:
dap_size:           db 0x10
dap_reserved1:      db 0
dap_count:          db 0
dap_reserved2:      db 0
dap_offset:         dw 0
dap_segment:        dw 0
dap_lba_low:        dd 0
dap_lba_high:       dd 0

mbr_buffer:         times 512 db 0

[BITS 32]
protected_mode_start:
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov ebp, 0x90000
    mov esp, ebp

    pop eax

    jmp 0x10000

    jmp $

times 5120-($-$$) db 0
