#include <myria/types.h>
#include <myria/kapi.h>

// LAPIC register offsets
#define LAPIC_ID                0x020
#define LAPIC_VERSION           0x030
#define LAPIC_TPR               0x080
#define LAPIC_APR               0x090
#define LAPIC_PPR               0x0A0
#define LAPIC_EOI               0x0B0
#define LAPIC_REMOTE_READ       0x0C0
#define LAPIC_LDR               0x0D0
#define LAPIC_DFR               0x0E0
#define LAPIC_SPURIOUS_VECTOR   0x0F0
#define LAPIC_ESR               0x280
#define LAPIC_ICR_LOW           0x300
#define LAPIC_ICR_HIGH          0x310
#define LAPIC_TIMER_LVT         0x320
#define LAPIC_THERMAL_LVT       0x330
#define LAPIC_PERF_LVT          0x340
#define LAPIC_LINT0_LVT         0x350
#define LAPIC_LINT1_LVT         0x360
#define LAPIC_ERROR_LVT         0x370
#define LAPIC_TIMER_ICR         0x380
#define LAPIC_TIMER_CCR         0x390
#define LAPIC_TIMER_DCR         0x3E0

// Default LAPIC base address
#define LAPIC_BASE_ADDR         0xFEE00000UL

// MSR for LAPIC base
#define MSR_APIC_BASE           0x1B

// LAPIC enable bit
#define LAPIC_ENABLE            (1 << 11)

// Timer modes
#define LAPIC_TIMER_PERIODIC    0x20000
#define LAPIC_TIMER_MASKED      0x10000

// Spurious interrupt vector
#define SPURIOUS_VECTOR         0xFF

static volatile u32 *lapic_base = (volatile u32*)LAPIC_BASE_ADDR;
static u32 timer_ticks_per_ms = 0;

// MSR functions are declared in kapi.h and implemented in msr.c

static u32 lapic_read(u32 reg) {
    return lapic_base[reg / 4];
}

static void lapic_write(u32 reg, u32 value) {
    lapic_base[reg / 4] = value;
}

// Timer interrupt handler
extern void timer_interrupt_handler(void);

void lapic_init(void) {
    // Check if LAPIC is available via CPUID
    u32 eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    
    if (!(edx & (1 << 9))) {
        kprintf("[LAPIC] LAPIC not supported by CPU, skipping\n");
        return;
    }
    
    // Enable LAPIC via MSR
    u64 apic_base = read_msr(MSR_APIC_BASE);
    apic_base |= LAPIC_ENABLE;
    write_msr(MSR_APIC_BASE, apic_base);
    
    // Map LAPIC to higher-half if needed (assuming identity mapped for now)
    
    // Enable LAPIC by setting spurious vector
    lapic_write(LAPIC_SPURIOUS_VECTOR, SPURIOUS_VECTOR | (1 << 8));
    
    // Clear error status register
    lapic_write(LAPIC_ESR, 0);
    lapic_write(LAPIC_ESR, 0);
    
    // Clear any pending interrupts
    lapic_write(LAPIC_EOI, 0);
    
    kprintf("[LAPIC] Local APIC initialized, ID=%u\n", lapic_read(LAPIC_ID) >> 24);
}

void timer_init(void) {
    // Check if LAPIC was initialized
    u32 eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    
    if (!(edx & (1 << 9))) {
        kprintf("[TIMER] LAPIC not available, using alternative timer\n");
        // TODO: Use PIT or other timer source
        return;
    }
    
    // First, we need to add the timer interrupt to IDT (vector 32)
    idt_set_gate(32, (u64)timer_interrupt_handler, 0x08, 0x8E);
    
    // Set timer divide configuration (divide by 16)
    lapic_write(LAPIC_TIMER_DCR, 0x03);
    
    // Calibrate timer frequency
    // For now, use a rough estimate - in real implementation, 
    // we'd calibrate against a known timer source
    
    // Set initial count for ~1ms (this is CPU frequency dependent)
    // For QEMU with modern CPU, rough estimate
    timer_ticks_per_ms = 1000000; // Adjust based on actual calibration
    
    // Set up timer interrupt (vector 32, periodic mode)
    lapic_write(LAPIC_TIMER_LVT, 32 | LAPIC_TIMER_PERIODIC);
    
    // Start timer with initial count
    lapic_write(LAPIC_TIMER_ICR, timer_ticks_per_ms);
    
    kprintf("[TIMER] APIC timer configured for 1ms ticks\n");
}

void lapic_eoi(void) {
    lapic_write(LAPIC_EOI, 0);
}