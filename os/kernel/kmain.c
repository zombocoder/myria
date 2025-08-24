#include <myria/types.h>
#include <myria/kapi.h>

// Minimal kernel test - no threading for now

void x86_early_init(void) {
    // Initialize serial port first for early logging
    serial_init();
    
    // Test serial output immediately
    serial_puts("Serial port initialized!\r\n");
    
    // Initialize kernel logging
    klog_init();
    
    kprintf("[BOOT] Myria OS starting...\n");
    kprintf("[BOOT] Early x86-64 initialization\n");
    
    // Check if we need to set up paging (Limine should have done this already)
    u64 cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    if (!(cr0 & (1UL << 31))) {
        // Paging not enabled, set it up
        kprintf("[BOOT] Setting up initial paging...\n");
        setup_initial_paging();
        enable_paging();
    } else {
        kprintf("[BOOT] Paging already enabled by bootloader\n");
    }
    
    // Set up GDT with proper kernel segments
    kprintf("[BOOT] Setting up GDT...\n");
    gdt_init();
    
    // Set up IDT with basic exception handlers
    kprintf("[BOOT] Setting up IDT...\n"); 
    idt_init();
    
    kprintf("[BOOT] Early initialization complete\n");
}

void kmain(void) {
    // Test raw serial output first
    serial_puts("[KERN] Entering kernel main via serial_puts\r\n");
    kprintf("[KERN] Entering kernel main\n");
    
    // Initialize local APIC
    kprintf("[KERN] Initializing LAPIC...\n");
    lapic_init();
    
    // Initialize timer (1ms ticks)
    kprintf("[KERN] Setting up timer...\n");
    timer_init();
    
    // Debug: Test basic functionality first
    kprintf("[KERN] Basic kernel functionality test\n");
    
    // Skip memory management and threading for now to isolate triple fault
    kprintf("[KERN] Skipping memory management initialization\n");
    
    // Enable interrupts
    kprintf("[KERN] Enabling interrupts...\n");
    sti();
    
    kprintf("[KERN] Minimal kernel test successful!\n");
    kprintf("[KERN] Kernel is now running - will print status every 2 seconds\n");
    
    // Heartbeat loop to show kernel is alive
    u64 last_time = 0;
    u64 counter = 0;
    while (1) {
        counter++;
        u64 current_time = get_system_time_ms();
        
        if (current_time - last_time >= 2000) {  // Every 2 seconds
            kprintf("[KERN] Heartbeat %lu - System uptime: %lu seconds\n", 
                    counter / 1000000, current_time / 1000);
            last_time = current_time;
        }
        
        // Small delay to prevent busy waiting
        for (volatile int i = 0; i < 1000; i++) {}
    }
}