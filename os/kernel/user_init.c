#include <myria/types.h>
#include <myria/kapi.h>


// User mode init program (embedded in kernel for now)
// This will be loaded into user address space and executed

// User syscall stub (inline assembly)
static inline u64 user_syscall(u64 syscall_num, u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5, u64 arg6) {
    register u64 rax __asm__("rax") = syscall_num;
    register u64 rdi __asm__("rdi") = arg1;
    register u64 rsi __asm__("rsi") = arg2;
    register u64 rdx __asm__("rdx") = arg3;
    register u64 r10 __asm__("r10") = arg4;
    register u64 r8  __asm__("r8")  = arg5;
    register u64 r9  __asm__("r9")  = arg6;
    
    __asm__ volatile("syscall"
                     : "+r"(rax)
                     : "r"(rdi), "r"(rsi), "r"(rdx), "r"(r10), "r"(r8), "r"(r9)
                     : "rcx", "r11", "memory");
    return rax;
}

// Simple user write function
void user_write(const char *msg) {
    u64 len = 0;
    // Calculate string length
    while (msg[len]) len++;
    
    // SYS_WRITE = 1, fd = 1 (stdout)
    user_syscall(1, 1, (u64)msg, len, 0, 0, 0);
}

// User init main function
void __attribute__((noreturn)) user_init_main(void) {
    // Print hello message from user mode
    user_write("Hello from user mode!\r\n");
    user_write("User mode init is running!\r\n");
    user_write("Testing syscalls from user space...\r\n");
    
    // Test getpid syscall
    u64 pid = user_syscall(7, 0, 0, 0, 0, 0, 0); // SYS_GETPID = 7
    (void)pid; // Suppress unused warning
    user_write("getpid() completed\r\n");
    
    // Test yield syscall
    user_write("Testing yield...\r\n");
    user_syscall(9, 0, 0, 0, 0, 0, 0); // SYS_YIELD = 9
    user_write("yield() completed\r\n");
    
    // Test sleep syscall
    user_write("Testing sleep(100ms)...\r\n");
    user_syscall(8, 100, 0, 0, 0, 0, 0); // SYS_SLEEP = 8
    user_write("sleep() completed\r\n");
    
    user_write("User init completed successfully!\r\n");
    
    // Exit user mode (SYS_EXIT = 0)
    user_syscall(0, 0, 0, 0, 0, 0, 0);
    
    // Should not reach here
    while (1) {
        __asm__ volatile("hlt");
    }
}

// Get the address and size of the user init program (simplified for now)
void get_user_init_info(void **code_start, u64 *code_size) {
    *code_start = (void*)user_init_main;
    *code_size = 4096; // Simplified size for now
}

// Simple user mode function that just loops and tries one syscall
void __attribute__((noreturn)) simple_user_loop(void) {
    // Try a simple syscall (write)
    __asm__ volatile(
        "mov $1, %%rax \n\t"    // SYS_WRITE = 1
        "mov $1, %%rdi \n\t"    // fd = 1 (stdout)
        "mov %0, %%rsi \n\t"    // message
        "mov $13, %%rdx \n\t"   // length
        "syscall"
        : : "r"("User mode OK\n") : "rax", "rdi", "rsi", "rdx", "rcx", "r11", "memory"
    );
    
    // Exit syscall
    __asm__ volatile(
        "mov $0, %%rax \n\t"    // SYS_EXIT = 0 (our kernel convention)  
        "mov $0, %%rdi \n\t"    // exit code = 0
        "syscall"
        : : : "rax", "rdi", "rcx", "r11", "memory"
    );
    
    // Should not reach here
    while (1) {
        __asm__ volatile("hlt");
    }
}

// Test user mode entry with proper user address space
void test_user_mode(void) {
    serial_puts("\r\n=== User Mode Test (Isolated Address Space) ===\r\n");
    
    // Create complete user process with separate PML4
    u64 user_pml4 = create_user_process();
    if (!user_pml4) {
        serial_puts("[USER] ERROR: Failed to create user process\r\n");
        return;
    }
    
    serial_puts("[USER] User process created with isolated address space\r\n");
    serial_puts("[USER] Code mapped at 0x10000 (RX, U=1)\r\n");
    serial_puts("[USER] Stack mapped at 0x800000-0x802000 (RW, NX, U=1)\r\n");
    serial_puts("[USER] Kernel high-half shared for syscalls\r\n");
    
    serial_puts("[USER] About to switch CR3 and enter user mode...\r\n");
    serial_puts("[USER] No more active page table modification - clean switch!\r\n");
    
    // Switch to isolated user address space and enter user mode
    // This eliminates all paging-structure cache issues
    switch_to_user_process(user_pml4);
    
    // Should never reach here
    serial_puts("[USER] ERROR: Returned from user mode!\r\n");
}