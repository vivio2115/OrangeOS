[BITS 32]
[GLOBAL tss_flush]

tss_flush:
    mov ax, [esp + 4]
    ltr ax
    ret