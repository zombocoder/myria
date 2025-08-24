.section .text

# GDT flush function
.global gdt_flush
.type gdt_flush, @function
gdt_flush:
    lgdt (%rdi)         # Load new GDT pointer
    
    # Reload code segment by doing a far return
    pushq $0x08         # Push new CS
    pushq $.reload_cs   # Push return address
    lretq               # Far return to reload CS
    
.reload_cs:
    # Reload data segments
    movw $0x10, %ax     # Kernel data segment selector
    movw %ax, %ds
    movw %ax, %es  
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss
    ret

# IDT flush function
.global idt_flush  
.type idt_flush, @function
idt_flush:
    lidt (%rdi)         # Load new IDT pointer
    ret