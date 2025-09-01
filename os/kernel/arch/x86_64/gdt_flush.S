# GDT and TSS flush assembly functions
.section .text
.intel_syntax noprefix

# void gdt_flush(u64 gdt_ptr);
.global gdt_flush
.type gdt_flush, @function
gdt_flush:
    # Load GDT
    lgdt [rdi]
    
    # Reload code segment by doing a far jump
    mov rax, offset reload_cs
    push 0x08  # Kernel code selector
    push rax
    retfq      # Far return to reload CS
    
reload_cs:
    # Reload data segments
    mov ax, 0x10    # Kernel data selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ret

# void tss_flush(u16 tss_selector);  
.global tss_flush
.type tss_flush, @function
tss_flush:
    ltr di  # Load Task Register with TSS selector
    ret

.att_syntax prefix