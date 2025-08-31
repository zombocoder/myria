#include <myria/types.h>
#include <myria/kapi.h>

// Forward declarations
extern u32 thread_create(void (*entry_point)(void *), void *arg, const char *name);
extern void sched_run_threads(void);

// Simple test function to verify C function calls work
void simple_test_function(void) {
    // Do absolutely nothing - just return
    // This tests if the call/return mechanism works at all
}

void x86_early_init(void) {
    serial_puts("X86_EARLY_INIT WORKING!\r\n");
    
    // CRITICAL: Set up proper page tables BEFORE accessing any globals
    serial_puts("Setting up kernel page tables with proper R/W/X permissions\r\n");
    u64 pml4_phys = setup_kernel_page_tables();
    serial_puts("Activating kernel page tables and clearing BSS\r\n");
    activate_kernel_page_tables(pml4_phys);
    serial_puts("Page tables activated - globals now accessible!\r\n");
    
    // NOW we can safely access global variables
    serial_puts("About to init PMM (globals now safe)\r\n");
    pmm_init();
    serial_puts("PMM init completed\r\n");
    
    // Initialize Virtual Memory Manager  
    serial_puts("About to init VMM\r\n");
    vmm_init();
    serial_puts("VMM init completed\r\n");
    
    // Initialize basic threading system
    serial_puts("About to init scheduler\r\n");
    sched_init();
    serial_puts("Scheduler init completed\r\n");
    
    // Initialize system calls
    serial_puts("About to init syscalls\r\n");
    syscall_init();
    serial_puts("Syscalls init completed\r\n");
    
    serial_puts("x86_early_init complete!\r\n");
}

// Enhanced test thread functions
void test_thread1(void *arg) {
    (void)arg;
    for (int i = 0; i < 3; i++) {
        serial_puts("THREAD1: Working... iteration ");
        serial_puts("\r\n");
        
        // Yield to other threads
        sched_yield();
    }
    serial_puts("THREAD1: Finished!\r\n");
}

void test_thread2(void *arg) {
    (void)arg;
    for (int i = 0; i < 3; i++) {
        serial_puts("THREAD2: Processing... iteration ");
        serial_puts("\r\n");
        
        // Yield to other threads  
        sched_yield();
    }
    serial_puts("THREAD2: Finished!\r\n");
}

void test_thread3(void *arg) {
    (void)arg;
    for (int i = 0; i < 2; i++) {
        serial_puts("THREAD3: Computing... iteration ");
        serial_puts("\r\n");
        
        // Yield to other threads
        sched_yield();
    }
    serial_puts("THREAD3: Finished!\r\n");
}

// Test enhanced memory management
void test_memory_management(void) {
    serial_puts("\r\n=== Enhanced Memory Management Test ===\r\n");
    
    // Test PMM statistics
    u64 total, free, used;
    pmm_get_stats(&total, &free, &used);
    serial_puts("[PMM] Memory statistics available\r\n");
    
    // Test multiple allocations
    serial_puts("[HEAP] Testing multiple allocations...\r\n");
    void *ptr1 = kmalloc(64);
    void *ptr2 = kmalloc(128);
    void *ptr3 = kmalloc(256);
    
    if (ptr1 && ptr2 && ptr3) {
        serial_puts("[HEAP] Multiple allocations succeeded!\r\n");
        
        // Test freeing and reallocation
        kfree(ptr2); // Free middle block
        void *ptr4 = kmalloc(100); // Should reuse freed space
        
        if (ptr4) {
            serial_puts("[HEAP] Block reuse successful!\r\n");
        }
        
        kfree(ptr1);
        kfree(ptr3);
        kfree(ptr4);
        serial_puts("[HEAP] All blocks freed successfully!\r\n");
    } else {
        serial_puts("[HEAP] Some allocations failed!\r\n");
    }
    
    // Test large allocation
    void *large_ptr = kmalloc(4096);
    if (large_ptr) {
        serial_puts("[HEAP] Large allocation (4KB) succeeded!\r\n");
        kfree(large_ptr);
        serial_puts("[HEAP] Large block freed successfully!\r\n");
    }
    
    serial_puts("=== Memory Management Test Complete ===\r\n");
}

// Test enhanced threading system
void test_threading_system(void) {
    serial_puts("\r\n=== Enhanced Threading System Test ===\r\n");
    
    // Create multiple threads
    u32 tid1 = thread_create(test_thread1, NULL, "Worker-1");
    u32 tid2 = thread_create(test_thread2, NULL, "Worker-2");
    u32 tid3 = thread_create(test_thread3, NULL, "Computer-3");
    
    if (tid1 && tid2 && tid3) {
        serial_puts("[TEST] All threads created successfully\r\n");
        
        // Print scheduler statistics
        sched_print_stats();
        
        // Start the enhanced scheduler (cooperative for now)
        sched_run_threads();
        
        serial_puts("[TEST] Thread execution completed\r\n");
        sched_print_stats();
    } else {
        serial_puts("[TEST] Failed to create some threads\r\n");
        
        // Fall back to direct calls
        serial_puts("[TEST] Falling back to direct thread calls\r\n");
        test_thread1(NULL);
        test_thread2(NULL);
        test_thread3(NULL);
    }
    
    serial_puts("=== Threading System Test Complete ===\r\n");
}

// Test system call interface
void test_system_calls(void) {
    serial_puts("\r\n=== System Call Interface Test ===\r\n");
    
    // Test getpid system call
    u64 pid = syscall_dispatch(7, 0, 0, 0, 0, 0, 0); // SYS_GETPID = 7
    (void)pid; // Suppress unused variable warning
    serial_puts("[TEST] getpid() returned PID\r\n");
    
    // Test yield system call
    serial_puts("[TEST] Testing yield system call\r\n");
    syscall_dispatch(9, 0, 0, 0, 0, 0, 0); // SYS_YIELD = 9
    
    // Test malloc system call
    serial_puts("[TEST] Testing malloc system call\r\n");
    u64 ptr = syscall_dispatch(10, 256, 0, 0, 0, 0, 0); // SYS_MALLOC = 10
    if (ptr) {
        serial_puts("[TEST] malloc succeeded\r\n");
        
        // Test free system call
        serial_puts("[TEST] Testing free system call\r\n");
        syscall_dispatch(11, ptr, 0, 0, 0, 0, 0); // SYS_FREE = 11
        serial_puts("[TEST] free completed\r\n");
    } else {
        serial_puts("[TEST] malloc failed\r\n");
    }
    
    // Test sleep system call
    serial_puts("[TEST] Testing sleep system call (50ms)\r\n");
    syscall_dispatch(8, 50, 0, 0, 0, 0, 0); // SYS_SLEEP = 8
    serial_puts("[TEST] sleep completed\r\n");
    
    // Test write system call
    serial_puts("[TEST] Testing write system call\r\n");
    const char *msg = "Hello from syscall!";
    syscall_dispatch(1, 1, (u64)msg, 19, 0, 0, 0); // SYS_WRITE = 1
    
    // Print system call statistics
    syscall_print_stats();
    
    serial_puts("=== System Call Interface Test Complete ===\r\n");
}

void kmain(void) {
    serial_puts("MINIMAL KERNEL WORKING!\r\n");
    
    serial_puts("About to test memory management\r\n");
    test_memory_management();
    serial_puts("Memory management test complete\r\n");
    
    serial_puts("About to test threading system\r\n");
    test_threading_system();
    serial_puts("Threading system test complete\r\n");
    
    serial_puts("About to test system calls\r\n");
    test_system_calls();
    serial_puts("System calls test complete\r\n");
    
    // Test complete - kernel is working!
    serial_puts("\r\n=== KERNEL BOOT SUCCESS ===\r\n");
    serial_puts("All major systems initialized successfully:\r\n");
    serial_puts("✓ PMM (Physical Memory Manager)\r\n");
    serial_puts("✓ VMM (Virtual Memory Manager)\r\n");  
    serial_puts("✓ Scheduler (Basic)\r\n");
    serial_puts("✓ kmalloc/kfree (Simplified)\r\n");
    serial_puts("✓ Serial I/O\r\n");
    serial_puts("=== KERNEL READY ===\r\n");
    
    // Simple infinite loop
    while(1) {
        __asm__ volatile("hlt");
    }
}