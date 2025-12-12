OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)

SECTIONS
{
    . = 0x40000000;
    
    .text ALIGN(0x1000) : {
        *(.text)
        *(.text.*)
    }
    
    .rodata ALIGN(0x1000) : {
        *(.rodata)
        *(.rodata.*)
    }
    
    .data ALIGN(0x1000) : {
        *(.data)
        *(.data.*)
    }
    
    .bss ALIGN(0x1000) : {
        *(.bss)
        *(.bss.*)
        *(COMMON)
    }
}