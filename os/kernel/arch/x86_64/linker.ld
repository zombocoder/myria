ENTRY(_start)

/* Program headers */
PHDRS {
    text PT_LOAD FLAGS(5);
}

SECTIONS {
    /* Standard higher-half kernel base */
    . = 0xffffffff80000000;
    
    /* Keep track of kernel start */
    __kernel_start = .;
    
    /* Code section */
    .text : ALIGN(4K) {
        __text_start = .;
        *(.text*)
        __text_end = .;
    } :text
    
    /* Limine requests section */
    .limine_reqs : {
        KEEP(*(.limine_reqs))
        KEEP(*(.requests*))  /* Keep old name too for compatibility */
    }
    
    /* Read-only data */
    .rodata : ALIGN(4K) {
        __rodata_start = .;
        *(.rodata*)
        __rodata_end = .;
    }
    
    /* Initialized data */  
    .data : ALIGN(4K) {
        __data_start = .;
        *(.data*)
        __data_end = .;
    }
    
    /* BSS (uninitialized data) */
    .bss : ALIGN(4K) {
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    }
    
    /* Kernel stack - separate section to ensure proper allocation */
    . = ALIGN(16);
    __stack_bottom = .;
    . += 0x4000;  /* 16KB stack */
    __stack_top = .;
    
    /* Keep track of kernel end */
    __kernel_end = .;
    
    
    /* Discard unneeded sections */
    /DISCARD/ : {
        *(.note*)
        *(.comment*)
        *(.eh_frame*)
    }
}