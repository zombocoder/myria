#include <myria/types.h>
#include <myria/kapi.h>

// GDT entry structure
struct gdt_entry {
    u16 limit_low;
    u16 base_low;
    u8  base_mid;
    u8  access;
    u8  granularity;
    u8  base_high;
} __attribute__((packed));

// TSS structure for x86_64
struct tss_entry {
    u32 reserved0;
    u64 rsp0;       // Stack pointer for ring 0
    u64 rsp1;       // Stack pointer for ring 1 (unused)
    u64 rsp2;       // Stack pointer for ring 2 (unused)
    u64 reserved1;
    u64 ist1;       // Interrupt Stack Table entry 1
    u64 ist2;       // IST entry 2
    u64 ist3;       // IST entry 3
    u64 ist4;       // IST entry 4
    u64 ist5;       // IST entry 5
    u64 ist6;       // IST entry 6
    u64 ist7;       // IST entry 7
    u64 reserved2;
    u16 reserved3;
    u16 iomap_base; // I/O map base address
} __attribute__((packed));

// GDT pointer structure
struct gdt_ptr {
    u16 limit;
    u64 base;
} __attribute__((packed));

// GDT selectors
#define KCODE_SEL 0x08
#define KDATA_SEL 0x10
#define UCODE_SEL 0x18
#define UDATA_SEL 0x20
#define TSS_SEL   0x28

// Access byte flags
#define GDT_PRESENT     (1 << 7)
#define GDT_DPL0        (0 << 5)
#define GDT_DPL3        (3 << 5)
#define GDT_SYSTEM      (0 << 4)
#define GDT_CODE_DATA   (1 << 4)
#define GDT_EXECUTABLE  (1 << 3)
#define GDT_DIRECTION   (0 << 2)  // 0=grows up, 1=grows down
#define GDT_READABLE    (1 << 1)  // for code segments
#define GDT_WRITABLE    (1 << 1)  // for data segments
#define GDT_ACCESSED    (1 << 0)

// Granularity flags
#define GDT_4K          (1 << 7)  // 4K granularity
#define GDT_32BIT       (1 << 6)  // 32-bit segment
#define GDT_64BIT       (1 << 5)  // 64-bit code segment
#define GDT_AVL         (1 << 4)  // Available for system use

// TSS type for system descriptor
#define TSS_TYPE_AVAILABLE  0x09

// Static GDT table (8 entries + TSS takes 2 entries = 10 total)
static struct gdt_entry gdt[10];
static struct gdt_ptr gdt_pointer;
static struct tss_entry tss __attribute__((section(".tss")));

// Kernel stacks for interrupts (8KB each)
static u8 kernel_stack[8192] __attribute__((aligned(16)));
static u8 interrupt_stack[8192] __attribute__((aligned(16)));

// External assembly function to load GDT and TSS
extern void gdt_flush(u64 gdt_ptr);
extern void tss_flush(u16 tss_selector);

// Set a GDT entry
static void gdt_set_entry(int index, u64 base, u64 limit, u8 access, u8 granularity) {
    gdt[index].base_low = base & 0xFFFF;
    gdt[index].base_mid = (base >> 16) & 0xFF;
    gdt[index].base_high = (base >> 24) & 0xFF;
    
    gdt[index].limit_low = limit & 0xFFFF;
    gdt[index].granularity = (limit >> 16) & 0x0F;
    gdt[index].granularity |= granularity & 0xF0;
    gdt[index].access = access;
}

// Set a TSS entry (takes 2 GDT entries in 64-bit mode)
static void gdt_set_tss(int index, u64 base, u64 limit, u8 access) {
    // TSS descriptors are 16 bytes in 64-bit mode
    gdt_set_entry(index, base, limit, access, 0);
    
    // Second part of TSS descriptor (high 32 bits of base)
    gdt[index + 1].limit_low = (base >> 32) & 0xFFFF;
    gdt[index + 1].base_low = (base >> 48) & 0xFFFF;
    gdt[index + 1].base_mid = 0;
    gdt[index + 1].access = 0;
    gdt[index + 1].granularity = 0;
    gdt[index + 1].base_high = 0;
}

// Initialize GDT with kernel and user segments
void gdt_init(void) {
    serial_puts("[GDT] Initializing Global Descriptor Table\r\n");
    
    // Clear GDT
    for (int i = 0; i < 10; i++) {
        gdt_set_entry(i, 0, 0, 0, 0);
    }
    
    // Entry 0: NULL descriptor (required)
    gdt_set_entry(0, 0, 0, 0, 0);
    
    // Entry 1: Kernel code segment (64-bit, DPL=0, R+X)
    gdt_set_entry(1, 0, 0xFFFFF, 
                  GDT_PRESENT | GDT_DPL0 | GDT_CODE_DATA | GDT_EXECUTABLE | GDT_READABLE,
                  GDT_4K | GDT_64BIT);
    
    // Entry 2: Kernel data segment (DPL=0, R+W, D/B=0 for long mode)
    gdt_set_entry(2, 0, 0xFFFFF,
                  GDT_PRESENT | GDT_DPL0 | GDT_CODE_DATA | GDT_WRITABLE,
                  GDT_4K);
    
    // Entry 3: User code segment (64-bit, DPL=3, R+X)
    gdt_set_entry(3, 0, 0xFFFFF,
                  GDT_PRESENT | GDT_DPL3 | GDT_CODE_DATA | GDT_EXECUTABLE | GDT_READABLE,
                  GDT_4K | GDT_64BIT);
    
    // Entry 4: User data segment (DPL=3, R+W, D/B=0 for long mode)
    gdt_set_entry(4, 0, 0xFFFFF,
                  GDT_PRESENT | GDT_DPL3 | GDT_CODE_DATA | GDT_WRITABLE,
                  GDT_4K);
    
    // Setup GDT pointer
    gdt_pointer.limit = sizeof(gdt) - 1;
    gdt_pointer.base = (u64)&gdt;
    
    serial_puts("[GDT] Loading GDT...\r\n");
    gdt_flush((u64)&gdt_pointer);
    
    serial_puts("[GDT] GDT loaded successfully\r\n");
}

// Initialize TSS for privilege level transitions
void tss_init(void) {
    serial_puts("[TSS] Initializing Task State Segment\r\n");
    
    // Clear TSS
    for (u64 i = 0; i < sizeof(tss); i++) {
        ((u8*)&tss)[i] = 0;
    }
    
    // Set kernel stack pointer (ring 0 stack)
    tss.rsp0 = (u64)(kernel_stack + sizeof(kernel_stack));
    
    // Set interrupt stack (IST1) for double faults, etc.
    tss.ist1 = (u64)(interrupt_stack + sizeof(interrupt_stack));
    
    // I/O map base (beyond TSS limit = no I/O map)
    tss.iomap_base = sizeof(tss);
    
    // Add TSS to GDT (entries 5-6, TSS takes 2 entries in 64-bit)
    gdt_set_tss(5, (u64)&tss, sizeof(tss) - 1, 
                GDT_PRESENT | GDT_DPL0 | TSS_TYPE_AVAILABLE);
    
    serial_puts("[TSS] Loading TSS...\r\n");
    tss_flush(TSS_SEL);
    
    serial_puts("[TSS] TSS loaded successfully\r\n");
}

// Get current kernel stack for syscall entry
u64 get_kernel_stack(void) {
    return tss.rsp0;
}

// Update kernel stack (for per-process kernel stacks later)
void set_kernel_stack(u64 stack_top) {
    tss.rsp0 = stack_top;
}

// Enter user mode for the first time
void enter_user(u64 rip, u64 rsp, u64 rflags) {
    serial_puts("[GDT] About to enter user mode\r\n");
    serial_puts("[GDT] User RIP: 0x10000\r\n"); 
    serial_puts("[GDT] User RSP: 0x802000\r\n");
    serial_puts("[GDT] User RFLAGS: 0x202\r\n");
    
    // Ensure stack is 16-byte aligned
    u64 aligned_rsp = rsp & ~0xF;
    
    // Debug segment selectors and clear debug state
    serial_puts("[GDT] CS=0x1B (UCODE|3), SS=0x23 (UDATA|3)\r\n");
    serial_puts("[GDT] Data segments now have D/B=0 for long mode\r\n");
    
    // COMPREHENSIVE: Clear ALL debug state to prevent Debug Exception
    serial_puts("[GDT] Clearing ALL debug state (DR0-DR7, IA32_DEBUGCTL)...\r\n");
    
    // Clear debug registers (including DR6 status register!)
    __asm__ volatile(
        "mov $0, %%rax\n\t"
        "mov %%rax, %%dr0\n\t"     // Clear breakpoint 0
        "mov %%rax, %%dr1\n\t"     // Clear breakpoint 1
        "mov %%rax, %%dr2\n\t"     // Clear breakpoint 2
        "mov %%rax, %%dr3\n\t"     // Clear breakpoint 3
        "mov %%rax, %%dr6\n\t"     // CRITICAL: Clear status (pending traps)
        "mov %%rax, %%dr7\n\t"     // CRITICAL: Set to 0 (NOT 0x400)
        : : : "rax"
    );
    
    // Clear IA32_DEBUGCTL MSR (Branch Trace Store, Last Branch Record, etc.)
    write_msr(0x1D9, 0); // IA32_DEBUGCTL = 0
    
    // Build clean RFLAGS: IF=1, TF=0, RF=1 (Resume Flag prevents pending #DB)
    #define RFLAGS_IF (1u << 9)   // Interrupt Flag
    #define RFLAGS_TF (1u << 8)   // Trap Flag  
    #define RFLAGS_RF (1u << 16)  // Resume Flag
    
    u64 clean_rflags = (rflags | RFLAGS_IF | RFLAGS_RF) & ~RFLAGS_TF;
    serial_puts("[GDT] Debug registers cleared, TF disabled\r\n");
    
    // Skip redundant TLB flush - caller already switched CR3 and flushed TLB
    serial_puts("[GDT] Skipping redundant TLB flush - using caller's CR3 setup\r\n");
    
    serial_puts("[GDT] Stack aligned, executing IRETQ...\r\n");
    
    // Use IRETQ to enter user mode (can't use SYSRET from kernel cold start)
    __asm__ volatile(
        "mov %0, %%rax \n\t"        // rsp
        "pushq %1       \n\t"       // SS (UDATA|3)
        "pushq %%rax    \n\t"       // RSP
        "pushq %2       \n\t"       // RFLAGS (with IF=1)
        "pushq %3       \n\t"       // CS (UCODE|3)
        "pushq %4       \n\t"       // RIP
        "iretq"
        : : "r"(aligned_rsp), "i"(UDATA_SEL|3), "r"(clean_rflags),
            "i"(UCODE_SEL|3), "r"(rip) : "rax","memory");
    
    // Should never reach here
    serial_puts("[GDT] ERROR: Return from user mode!\r\n");
    for (;;) {
        __asm__ volatile("cli; hlt");
    }
}