OUTPUT_FORMAT(elf32-i386)
ENTRY(start)

SECTIONS {
    /* Ядро загружается по адресу 1MB */
    . = 0x00100000;
    
    .text : {
        *(.multiboot)
        *(.text)
    }
    
    .rodata : {
        *(.rodata)
    }
    
    .data : {
        *(.data)
    }
    
    .bss : {
        *(COMMON)
        *(.bss)
        *(.bootstrap_stack)
    }
}
