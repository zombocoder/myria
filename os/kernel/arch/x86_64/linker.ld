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
    
    /* Code section with Limine requests at start */
    .text : ALIGN(4K) {
        __text_start = .;
        KEEP(*(.requests*))
        *(.text*)
        __text_end = .;
    } :text
    
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
        *(.bss*)
        *(COMMON)
        __bss_end = .;
    }
    
    /* Kernel stack */
    . = ALIGN(4K);
    __stack_bottom = .;
    . += 0x2000;  /* 8KB stack - minimal for early boot */
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