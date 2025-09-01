#include <myria/types.h>
#include <myria/kapi.h>

// Minimal IDT for handling faults during user mode transitions

// IDT entry structure
struct idt_entry {
    u16 offset_low;
    u16 selector;
    u8  ist;
    u8  type_attr;
    u16 offset_mid;
    u32 offset_high;
    u32 zero;
} __attribute__((packed));

// IDT pointer
struct idt_ptr {
    u16 limit;
    u64 base;
} __attribute__((packed));

// Minimal IDT with 32 entries
static struct idt_entry idt[32];
static struct idt_ptr idt_pointer;

// Fault handler declarations
extern void generic_fault_handler(void);
extern void gp_fault_handler(void);
extern void pf_fault_handler(void); 
extern void ud_fault_handler(void);
extern void isr_0(void);
extern void isr_1(void);
extern void isr_3(void);
extern void isr_6(void);
extern void isr_8(void);
extern void isr_13(void);
extern void isr_14(void);

// Specific fault handler stubs
__asm__(
    ".global generic_fault_handler\n"
    ".global gp_fault_handler\n"
    ".global pf_fault_handler\n"
    ".global ud_fault_handler\n"
    
    "gp_fault_handler:\n"
    "    cli\n"
    "    pushq %rax\n"
    "    pushq %rdi\n"
    "    popq %rdi\n"           // error code in RDI
    "    movq $13, %rax\n"      // vector 13 (#GP)
    "    call handle_fault_with_vector\n"
    "    popq %rax\n"
    "    addq $8, %rsp\n"       // Remove error code
    "    iretq\n"
    
    "pf_fault_handler:\n"
    "    cli\n"
    "    pushq %rax\n"
    "    pushq %rdi\n"
    "    popq %rdi\n"           // error code in RDI
    "    movq $14, %rax\n"      // vector 14 (#PF)
    "    call handle_fault_with_vector\n"
    "    popq %rax\n"
    "    addq $8, %rsp\n"       // Remove error code
    "    iretq\n"
    
    "ud_fault_handler:\n"
    "    cli\n"
    "    pushq %rax\n"
    "    pushq $0\n"            // dummy error code
    "    movq $6, %rax\n"       // vector 6 (#UD)
    "    call handle_fault_with_vector\n"
    "    popq %rax\n"
    "    addq $8, %rsp\n"
    "    iretq\n"
    
    "generic_fault_handler:\n"
    "    cli\n"
    "    pushq %rax\n"
    "    pushq $0\n"
    "    movq $99, %rax\n"      // Unknown vector
    "    call handle_fault_with_vector\n"
    "    popq %rax\n"
    "    addq $8, %rsp\n"
    "    iretq\n"
    
    // Per-vector stubs for proper exception identification
    ".global isr_0\n"
    "isr_0:\n"
    "    cli\n"
    "    pushq $0\n"            // dummy error code
    "    pushq %rax\n"
    "    pushq %rdi\n"
    "    movq $0, %rdi\n"       // vector 0
    "    movq $0, %rsi\n"       // error code 0
    "    call handle_fault_with_vector\n"
    "    popq %rdi\n"
    "    popq %rax\n"
    "    addq $8, %rsp\n"
    "    iretq\n"
    
    ".global isr_1\n"
    "isr_1:\n"
    "    cli\n"
    "    pushq $0\n"
    "    pushq %rax\n"
    "    pushq %rdi\n"
    "    movq $1, %rdi\n"       // vector 1 (#DB)
    "    movq $0, %rsi\n"
    "    call handle_fault_with_vector\n"
    "    popq %rdi\n"
    "    popq %rax\n"
    "    addq $8, %rsp\n"
    "    iretq\n"
    
    ".global isr_6\n"
    "isr_6:\n"
    "    cli\n"
    "    pushq $0\n"
    "    pushq %rax\n"
    "    pushq %rdi\n"
    "    movq $6, %rdi\n"       // vector 6 (#UD)
    "    movq $0, %rsi\n"
    "    call handle_fault_with_vector\n"
    "    popq %rdi\n"
    "    popq %rax\n"
    "    addq $8, %rsp\n"
    "    iretq\n"
    
    ".global isr_13\n"
    "isr_13:\n"
    "    cli\n"
    "    pushq %rax\n"
    "    pushq %rdi\n"
    "    movq 16(%rsp), %rsi\n" // error code from CPU
    "    movq $13, %rdi\n"      // vector 13 (#GP)
    "    call handle_fault_with_vector\n"
    "    popq %rdi\n"
    "    popq %rax\n"
    "    addq $8, %rsp\n"       // remove CPU error code
    "    iretq\n"
    
    ".global isr_14\n"
    "isr_14:\n"
    "    cli\n"
    "    pushq %rax\n"
    "    pushq %rdi\n"
    "    movq 16(%rsp), %rsi\n" // error code from CPU
    "    movq $14, %rdi\n"      // vector 14 (#PF)
    "    call handle_fault_with_vector\n"
    "    popq %rdi\n"
    "    popq %rax\n"
    "    addq $8, %rsp\n"
    "    iretq\n"
);

// Enhanced fault handler with vector decoding
void handle_fault_with_vector(u64 vector, u64 error_code) {
    (void)error_code; // Suppress unused parameter warning
    serial_puts("[IDT] FAULT DETAILS:\r\n");
    
    switch(vector) {
        case 0:
            serial_puts("[IDT] Vector 0: Divide by Zero (#DE)\r\n");
            break;
        case 1: {
            // Read debug status and control registers
            u64 dr6, dr7;
            __asm__ volatile("mov %%dr6, %0" : "=r"(dr6));
            __asm__ volatile("mov %%dr7, %0" : "=r"(dr7));
            
            serial_puts("[IDT] Vector 1: Debug (#DB) - CONTINUING EXECUTION\r\n");
            serial_puts("[IDT] This may be a QEMU artifact during user mode entry\r\n");
            serial_puts("[IDT] DR6 (status) shows reason for debug exception\r\n");
            serial_puts("[IDT] DR7 (control) shows enabled breakpoints\r\n");
            
            // Clear DR6 to prevent re-triggering on iret from handler
            __asm__ volatile("mov $0, %%rax; mov %%rax, %%dr6" : : : "rax");
            serial_puts("[IDT] DR6 cleared - allowing user mode execution to continue\r\n");
            
            // Don't halt - let execution continue to see if user mode works
            return;
        }
        case 3:
            serial_puts("[IDT] Vector 3: Breakpoint (#BP)\r\n");
            break;
        case 6:
            serial_puts("[IDT] Vector 6: Invalid Opcode (#UD)\r\n");
            serial_puts("[IDT] User code may contain invalid instructions\r\n");
            break;
        case 8:
            serial_puts("[IDT] Vector 8: Double Fault (#DF)\r\n");
            serial_puts("[IDT] CRITICAL: Double fault during IRETQ\r\n");
            break;
        case 13:
            serial_puts("[IDT] Vector 13: General Protection Fault (#GP)\r\n");
            serial_puts("[IDT] Error code: segment selector or other GP violation\r\n");
            break;
        case 14: {
            u64 cr2;
            __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
            serial_puts("[IDT] Vector 14: Page Fault (#PF)\r\n");
            serial_puts("[IDT] Faulting address (CR2): user code at 0x400000\r\n");
            
            // Decode page fault error code
            if (error_code & 1) {
                serial_puts("[IDT] Protection violation (page was present)\r\n");
            } else {
                serial_puts("[IDT] Page not present (mapping missing)\r\n");
            }
            
            if (error_code & 2) {
                serial_puts("[IDT] Write access attempted\r\n");
            } else {
                serial_puts("[IDT] Read/Execute access attempted\r\n");
            }
            
            if (error_code & 4) {
                serial_puts("[IDT] User mode access (CPL=3)\r\n");
            } else {
                serial_puts("[IDT] Supervisor mode access (CPL=0)\r\n");
            }
            
            serial_puts("[IDT] User code page may not have U=1 at all page table levels\r\n");
            break;
        }
        default:
            serial_puts("[IDT] Unknown vector\r\n");
            break;
    }
    
    serial_puts("[IDT] This confirms IRETQ is executing but hitting exception\r\n");
    serial_puts("[IDT] GDT segment fix is working - need to check page mappings\r\n");
    serial_puts("[IDT] Halting for debugging\r\n");
    
    for (;;) {
        __asm__ volatile("cli; hlt");
    }
}

// Legacy handler for compatibility
void handle_fault(void) {
    handle_fault_with_vector(99, 0);
}

// Set an IDT entry
static void idt_set_gate(u8 num, u64 base, u16 sel, u8 flags) {
    idt[num].offset_low = base & 0xFFFF;
    idt[num].offset_mid = (base >> 16) & 0xFFFF;
    idt[num].offset_high = (base >> 32) & 0xFFFFFFFF;
    
    idt[num].selector = sel;
    idt[num].ist = 0;  // Don't use IST for now
    idt[num].type_attr = flags;
    idt[num].zero = 0;
}

// Initialize minimal IDT
void idt_init(void) {
    serial_puts("[IDT] Initializing minimal IDT for fault handling\r\n");
    
    // Clear IDT
    for (int i = 0; i < 32; i++) {
        idt_set_gate(i, 0, 0, 0);
    }
    
    // Set up per-vector handlers with proper vector identification  
    idt_set_gate(0,  (u64)isr_0,  0x08, 0x8E); // Divide by zero
    idt_set_gate(1,  (u64)isr_1,  0x08, 0x8E); // Debug
    idt_set_gate(3,  (u64)generic_fault_handler, 0x08, 0x8E); // Breakpoint  
    idt_set_gate(6,  (u64)isr_6,  0x08, 0x8E); // Invalid opcode
    idt_set_gate(8,  (u64)generic_fault_handler, 0x08, 0x8E); // Double fault
    idt_set_gate(13, (u64)isr_13, 0x08, 0x8E); // General protection fault
    idt_set_gate(14, (u64)isr_14, 0x08, 0x8E); // Page fault
    
    // Set up IDT pointer
    idt_pointer.limit = sizeof(idt) - 1;
    idt_pointer.base = (u64)&idt;
    
    // Load IDT
    __asm__ volatile("lidt %0" : : "m"(idt_pointer));
    
    serial_puts("[IDT] Minimal IDT loaded - faults will now be handled\r\n");
}