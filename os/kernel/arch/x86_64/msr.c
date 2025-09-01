#include <myria/types.h>
#include <myria/kapi.h>

// MSR numbers
#define MSR_EFER        0xC0000080
#define MSR_STAR        0xC0000081
#define MSR_LSTAR       0xC0000082
#define MSR_CSTAR       0xC0000083
#define MSR_SFMASK      0xC0000084
#define MSR_KERNEL_GS   0xC0000102

// EFER flags
#define EFER_SCE        (1 << 0)   // System Call Extensions
#define EFER_LME        (1 << 8)   // Long Mode Enable
#define EFER_LMA        (1 << 10)  // Long Mode Active
#define EFER_NXE        (1 << 11)  // No-Execute Enable

// RFLAGS to mask on syscall entry
#define RFLAGS_IF       (1 << 9)   // Interrupt flag
#define RFLAGS_DF       (1 << 10)  // Direction flag
#define RFLAGS_TF       (1 << 8)   // Trap flag

// Read Model Specific Register
u64 read_msr(u32 msr) {
    u32 low, high;
    __asm__ volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return ((u64)high << 32) | low;
}

// Write Model Specific Register
void write_msr(u32 msr, u64 value) {
    u32 low = (u32)value;
    u32 high = (u32)(value >> 32);
    __asm__ volatile("wrmsr" : : "a"(low), "d"(high), "c"(msr));
}

// Enable user mode features in control registers
static void setup_user_mode_cr_flags(void) {
    // Enable Write Protect (WP) in CR0 - kernel respects user page permissions
    u64 cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= (1 << 16); // WP bit
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
    
    // Enable Page Global Enable (PGE) in CR4 for better TLB performance
    u64 cr4;
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1 << 7);  // PGE bit
    __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));
    
    serial_puts("[MSR] User mode CR0/CR4 flags enabled\r\n");
}

// Configure SYSCALL/SYSRET MSRs
void setup_syscall_msrs(void) {
    serial_puts("[MSR] Setting up SYSCALL/SYSRET MSRs\r\n");
    
    // Setup control registers for user mode
    setup_user_mode_cr_flags();
    
    // Enable System Call Extensions in EFER
    u64 efer = read_msr(MSR_EFER);
    efer |= EFER_SCE | EFER_NXE;  // Enable syscall and NX bit
    write_msr(MSR_EFER, efer);
    
    // STAR: Segment selectors for syscall/sysret
    // Bits 63:48 = User CS base (UCODE - 16)
    // Bits 47:32 = Kernel CS base (KCODE) 
    u64 star = ((u64)0x18 << 48) | ((u64)0x08 << 32);  // User=0x18, Kernel=0x08
    write_msr(MSR_STAR, star);
    
    // LSTAR: Kernel entry point for syscall
    extern void syscall_entry(void);
    write_msr(MSR_LSTAR, (u64)syscall_entry);
    
    // SFMASK: RFLAGS mask (clear these flags on syscall entry)
    write_msr(MSR_SFMASK, RFLAGS_IF | RFLAGS_DF | RFLAGS_TF);
    
    // Optionally set KERNEL_GS for per-CPU data (not implemented yet)
    // write_msr(MSR_KERNEL_GS, (u64)&percpu_data);
    
    serial_puts("[MSR] SYSCALL/SYSRET MSRs configured\r\n");
}