ENTRY(_start)

/* Program headers for W^X enforcement */
PHDRS {
    text PT_LOAD FLAGS(5);    /* R+X */
    rodata PT_LOAD FLAGS(4);  /* R */
    data PT_LOAD FLAGS(6);    /* R+W */
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
    } :rodata
    
    /* Initialized data */  
    .data : ALIGN(4K) {
        __data_start = .;
        *(.data*)
        *(.sdata*)
        __data_end = .;
    } :data
    
    /* BSS (uninitialized data) */
    .bss : ALIGN(4K) {
        __bss_start = .;
        *(.bss .bss.*)
        *(.sbss .sbss.*)
        *(COMMON)
        __bss_end = .;
    } :data
    
    /* TSS and per-CPU data */
    .percpu : ALIGN(4K) {
        __percpu_start = .;
        *(.percpu*)
        *(.tss*)
        __percpu_end = .;
    } :data
    
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