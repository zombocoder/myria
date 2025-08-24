#include <myria/types.h>
#include <myria/kapi.h>

// GDT Entry structure
struct gdt_entry {
    u16 limit_low;
    u16 base_low;
    u8  base_middle;
    u8  access;
    u8  granularity;
    u8  base_high;
} PACKED;

// GDT Pointer structure
struct gdt_ptr {
    u16 limit;
    u64 base;
} PACKED;

// IDT Entry structure  
struct idt_entry {
    u16 base_low;
    u16 selector;
    u8  ist;
    u8  flags;
    u16 base_middle;
    u32 base_high;
    u32 reserved;
} PACKED;

// IDT Pointer structure
struct idt_ptr {
    u16 limit;
    u64 base;
} PACKED;

// GDT entries - initialized to eliminate BSS
static struct gdt_entry gdt[5] = {0};
static struct gdt_ptr gdt_pointer = {0};

// IDT entries - initialized to eliminate BSS
static struct idt_entry idt[256] = {0};
static struct idt_ptr idt_pointer = {0};

// External assembly functions
extern void gdt_flush(u64 gdt_ptr);
extern void idt_flush(u64 idt_ptr);

// Forward declaration for idt_set_gate (used by lapic.c)
void idt_set_gate(u8 num, u64 base, u16 sel, u8 flags);

// Exception handlers (declared in start.S)
extern void divide_error_handler(void);
extern void debug_handler(void);
extern void nmi_handler(void);
extern void breakpoint_handler(void);
extern void overflow_handler(void);
extern void bound_range_handler(void);
extern void invalid_opcode_handler(void);
extern void device_not_available_handler(void);
extern void double_fault_handler(void);
extern void invalid_tss_handler(void);
extern void segment_not_present_handler(void);
extern void stack_segment_fault_handler(void);
extern void general_protection_handler(void);
extern void page_fault_handler(void);
extern void x87_fpu_error_handler(void);
extern void alignment_check_handler(void);
extern void machine_check_handler(void);
extern void simd_fpu_handler(void);
extern void virtualization_handler(void);

static void gdt_set_gate(int num, u64 base, u32 limit, u8 access, u8 gran) {
    gdt[num].base_low    = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high   = (base >> 24) & 0xFF;
    
    gdt[num].limit_low   = (limit & 0xFFFF);
    gdt[num].granularity = (limit >> 16) & 0x0F;
    
    gdt[num].granularity |= gran & 0xF0;
    gdt[num].access      = access;
}

void idt_set_gate(u8 num, u64 base, u16 sel, u8 flags) {
    idt[num].base_low    = base & 0xFFFF;
    idt[num].base_middle = (base >> 16) & 0xFFFF;
    idt[num].base_high   = (base >> 32) & 0xFFFFFFFF;
    
    idt[num].selector = sel;
    idt[num].reserved = 0;
    idt[num].ist      = 0;  // No IST for now
    idt[num].flags    = flags;
}

void gdt_init(void) {
    gdt_pointer.limit = (sizeof(struct gdt_entry) * 5) - 1;
    gdt_pointer.base = (u64)&gdt;
    
    // NULL descriptor
    gdt_set_gate(0, 0, 0, 0, 0);
    
    // Kernel code segment (64-bit)
    gdt_set_gate(1, 0, 0xFFFFF, 0x9A, 0xA0);
    
    // Kernel data segment 
    gdt_set_gate(2, 0, 0xFFFFF, 0x92, 0xA0);
    
    // User code segment (will be used later)
    gdt_set_gate(3, 0, 0xFFFFF, 0xFA, 0xA0);
    
    // User data segment (will be used later)
    gdt_set_gate(4, 0, 0xFFFFF, 0xF2, 0xA0);
    
    gdt_flush((u64)&gdt_pointer);
}

void idt_init(void) {
    idt_pointer.limit = (sizeof(struct idt_entry) * 256) - 1;
    idt_pointer.base = (u64)&idt;
    
    // Clear IDT
    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0, 0);
    }
    
    // Set up exception handlers
    idt_set_gate(0,  (u64)divide_error_handler,           0x08, 0x8E);
    idt_set_gate(1,  (u64)debug_handler,                  0x08, 0x8E);
    idt_set_gate(2,  (u64)nmi_handler,                    0x08, 0x8E);
    idt_set_gate(3,  (u64)breakpoint_handler,             0x08, 0x8E);
    idt_set_gate(4,  (u64)overflow_handler,               0x08, 0x8E);
    idt_set_gate(5,  (u64)bound_range_handler,            0x08, 0x8E);
    idt_set_gate(6,  (u64)invalid_opcode_handler,         0x08, 0x8E);
    idt_set_gate(7,  (u64)device_not_available_handler,   0x08, 0x8E);
    idt_set_gate(8,  (u64)double_fault_handler,           0x08, 0x8E);
    idt_set_gate(10, (u64)invalid_tss_handler,            0x08, 0x8E);
    idt_set_gate(11, (u64)segment_not_present_handler,    0x08, 0x8E);
    idt_set_gate(12, (u64)stack_segment_fault_handler,    0x08, 0x8E);
    idt_set_gate(13, (u64)general_protection_handler,     0x08, 0x8E);
    idt_set_gate(14, (u64)page_fault_handler,             0x08, 0x8E);
    idt_set_gate(16, (u64)x87_fpu_error_handler,          0x08, 0x8E);
    idt_set_gate(17, (u64)alignment_check_handler,        0x08, 0x8E);
    idt_set_gate(18, (u64)machine_check_handler,          0x08, 0x8E);
    idt_set_gate(19, (u64)simd_fpu_handler,               0x08, 0x8E);
    idt_set_gate(20, (u64)virtualization_handler,         0x08, 0x8E);
    
    idt_flush((u64)&idt_pointer);
}