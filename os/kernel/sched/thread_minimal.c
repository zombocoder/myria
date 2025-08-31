#include <myria/types.h>
#include <myria/kapi.h>

// Forward declare serial functions
extern void serial_puts(const char *str);

// Thread states
typedef enum {
    THREAD_STATE_READY = 0,
    THREAD_STATE_RUNNING,
    THREAD_STATE_BLOCKED,
    THREAD_STATE_ZOMBIE
} thread_state_t;

// CPU context for context switching
typedef struct {
    u64 rax, rbx, rcx, rdx;
    u64 rsi, rdi, rbp, rsp;
    u64 r8, r9, r10, r11;
    u64 r12, r13, r14, r15;
    u64 rip, rflags;
} cpu_context_t;

// Enhanced Thread Control Block
struct thread {
    u32 tid;                    
    thread_state_t state;
    u8 priority;
    char name[32];
    void (*entry_point)(void*);
    void *arg;
    void *stack_base;         // Base of stack
    void *stack_top;          // Top of stack  
    cpu_context_t context;    // Saved CPU context
    u64 time_slice;           // Time slice counter
    u64 total_runtime;        // Total execution time
};
typedef struct thread thread_t;

// Forward declare assembly context switching functions  
extern void switch_to_thread(cpu_context_t *old_ctx, cpu_context_t *new_ctx);
extern void thread_bootstrap(void);

// Forward declare internal functions
static void thread_wrapper(void);
static thread_t *find_next_thread(void);
static void cleanup_zombie_threads(void);

// Enhanced scheduler state  
#define MAX_THREADS 8
#define THREAD_STACK_SIZE 8192
#define DEFAULT_TIME_SLICE 10

static thread_t thread_table[MAX_THREADS];
static thread_t *current_thread = NULL;
static u32 next_tid = 1;
static u32 active_thread_count = 0;
static u32 scheduler_ticks = 0;
static bool scheduler_enabled = false;

// Initialize the enhanced threading system
void sched_init(void) {
    serial_puts("[SCHED] Initializing enhanced scheduler\r\n");
    
    // Initialize scheduler state
    current_thread = NULL;
    next_tid = 1;
    active_thread_count = 0;
    scheduler_ticks = 0;
    scheduler_enabled = false;
    
    // Clear thread table
    for (int i = 0; i < MAX_THREADS; i++) {
        thread_table[i].tid = 0;
        thread_table[i].state = THREAD_STATE_ZOMBIE;
        thread_table[i].priority = 1;
        thread_table[i].entry_point = NULL;
        thread_table[i].arg = NULL;
        thread_table[i].stack_base = NULL;
        thread_table[i].stack_top = NULL;
        thread_table[i].time_slice = 0;
        thread_table[i].total_runtime = 0;
        
        // Clear name
        for (int j = 0; j < 32; j++) {
            thread_table[i].name[j] = 0;
        }
    }
    
    serial_puts("[SCHED] Thread table initialized\r\n");
    serial_puts("[SCHED] Enhanced scheduler initialized\r\n");
}

// Get current thread (stub)
thread_t *sched_current_thread(void) {
    return current_thread;
}

// Enhanced yield function with context switching
void sched_yield(void) {
    if (!scheduler_enabled || !current_thread) {
        return; // No-op if scheduler not ready
    }
    
    // Find next thread to run
    thread_t *next = find_next_thread();
    if (!next || next == current_thread) {
        return; // No other thread to switch to
    }
    
    // Perform context switch (this would use assembly context switching)
    // For now, implement cooperative switching
    thread_t *old_thread = current_thread;
    current_thread = next;
    next->state = THREAD_STATE_RUNNING;
    old_thread->state = THREAD_STATE_READY;
    
    serial_puts("[SCHED] Context switch to: ");
    serial_puts(next->name);
    serial_puts("\r\n");
}

// Enhanced tick function with preemptive scheduling
void sched_tick(void) {
    if (!scheduler_enabled) return;
    
    scheduler_ticks++;
    
    // Update current thread's runtime
    if (current_thread) {
        current_thread->total_runtime++;
        current_thread->time_slice--;
        
        // Check if time slice expired
        if (current_thread->time_slice <= 0) {
            current_thread->time_slice = DEFAULT_TIME_SLICE;
            sched_yield(); // Force context switch
        }
    }
    
    // Periodic cleanup of zombie threads
    if (scheduler_ticks % 100 == 0) {
        cleanup_zombie_threads();
    }
}

// Create an enhanced kernel thread with proper context setup
u32 thread_create(void (*entry_point)(void *), void *arg, const char *name) {
    if (!entry_point) return 0;
    
    serial_puts("[SCHED] Creating thread: ");
    if (name) serial_puts(name);
    serial_puts("\r\n");
    
    // Find free slot
    int slot = -1;
    for (int i = 0; i < MAX_THREADS; i++) {
        if (thread_table[i].tid == 0) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        serial_puts("[SCHED] No free thread slots available\r\n");
        return 0;
    }
    
    // Allocate stack using kmalloc
    void *stack_base = kmalloc(THREAD_STACK_SIZE);
    if (!stack_base) {
        serial_puts("[SCHED] Failed to allocate thread stack\r\n");
        return 0;
    }
    
    // Calculate stack top (stacks grow downward)
    void *stack_top = (u8*)stack_base + THREAD_STACK_SIZE;
    
    // Initialize thread structure
    thread_table[slot].tid = next_tid++;
    thread_table[slot].state = THREAD_STATE_READY;
    thread_table[slot].priority = 1;
    thread_table[slot].entry_point = entry_point;
    thread_table[slot].arg = arg;
    thread_table[slot].stack_base = stack_base;
    thread_table[slot].stack_top = stack_top;
    thread_table[slot].time_slice = DEFAULT_TIME_SLICE;
    thread_table[slot].total_runtime = 0;
    
    // Set thread name
    for (int i = 0; i < 32; i++) thread_table[slot].name[i] = 0;
    if (name) {
        for (int i = 0; name[i] && i < 31; i++) {
            thread_table[slot].name[i] = name[i];
        }
    }
    
    // Initialize context for first run
    // Set stack pointer to top of stack (16-byte aligned)
    u64 aligned_stack = ((u64)stack_top - 16) & ~0xF;
    thread_table[slot].context.rsp = aligned_stack;
    thread_table[slot].context.rbp = aligned_stack;
    thread_table[slot].context.rip = (u64)thread_wrapper;
    thread_table[slot].context.rflags = 0x202; // Enable interrupts
    
    // Clear other registers
    thread_table[slot].context.rax = 0;
    thread_table[slot].context.rbx = 0;
    thread_table[slot].context.rcx = 0;
    thread_table[slot].context.rdx = 0;
    thread_table[slot].context.rsi = 0;
    thread_table[slot].context.rdi = 0;
    thread_table[slot].context.r8 = 0;
    thread_table[slot].context.r9 = 0;
    thread_table[slot].context.r10 = 0;
    thread_table[slot].context.r11 = 0;
    thread_table[slot].context.r12 = 0;
    thread_table[slot].context.r13 = 0;
    thread_table[slot].context.r14 = 0;
    thread_table[slot].context.r15 = 0;
    
    active_thread_count++;
    
    serial_puts("[SCHED] Thread created with TID: ");
    serial_puts("\r\n");
    
    return thread_table[slot].tid;
}

// Start the enhanced scheduler 
void sched_start(void) {
    serial_puts("[SCHED] Starting enhanced scheduler\r\n");
    
    scheduler_enabled = true;
    
    // Find the first ready thread and start executing
    thread_t *first_thread = find_next_thread();
    if (first_thread) {
        current_thread = first_thread;
        first_thread->state = THREAD_STATE_RUNNING;
        
        serial_puts("[SCHED] Starting first thread: ");
        serial_puts(first_thread->name);
        serial_puts("\r\n");
    } else {
        serial_puts("[SCHED] No threads ready to run\r\n");
    }
}

// Run threads cooperatively (backward compatibility)
void sched_run_threads(void) {
    serial_puts("[SCHED] Running threads in compatibility mode\r\n");
    
    for (int i = 0; i < MAX_THREADS; i++) {
        if (thread_table[i].tid != 0 && thread_table[i].state == THREAD_STATE_READY) {
            serial_puts("[SCHED] Executing thread: ");
            serial_puts(thread_table[i].name);
            serial_puts("\r\n");
            
            // Simple direct execution (no context switching)
            current_thread = &thread_table[i];
            thread_table[i].state = THREAD_STATE_RUNNING;
            thread_table[i].entry_point(thread_table[i].arg);
            thread_table[i].state = THREAD_STATE_ZOMBIE;
            current_thread = NULL;
        }
    }
    
    serial_puts("[SCHED] All threads completed\r\n");
}

// Thread wrapper function - called when thread starts
static void thread_wrapper(void) {
    if (current_thread && current_thread->entry_point) {
        // Call the actual thread function
        current_thread->entry_point(current_thread->arg);
        
        // Thread finished - mark as zombie
        current_thread->state = THREAD_STATE_ZOMBIE;
        active_thread_count--;
        
        serial_puts("[SCHED] Thread finished: ");
        serial_puts(current_thread->name);
        serial_puts("\r\n");
        
        // Yield to next thread
        sched_yield();
    }
    
    // Should not reach here, but if we do, hang
    while (1) {
        __asm__ volatile("hlt");
    }
}

// Find next ready thread (round-robin scheduling)
static thread_t *find_next_thread(void) {
    if (active_thread_count == 0) return NULL;
    
    // Start searching from current thread + 1
    int start_idx = 0;
    if (current_thread) {
        for (int i = 0; i < MAX_THREADS; i++) {
            if (&thread_table[i] == current_thread) {
                start_idx = (i + 1) % MAX_THREADS;
                break;
            }
        }
    }
    
    // Look for next ready thread
    for (int i = 0; i < MAX_THREADS; i++) {
        int idx = (start_idx + i) % MAX_THREADS;
        if (thread_table[idx].tid != 0 && thread_table[idx].state == THREAD_STATE_READY) {
            return &thread_table[idx];
        }
    }
    
    return NULL;
}

// Clean up finished threads
static void cleanup_zombie_threads(void) {
    for (int i = 0; i < MAX_THREADS; i++) {
        if (thread_table[i].state == THREAD_STATE_ZOMBIE && thread_table[i].tid != 0) {
            // Free the stack
            if (thread_table[i].stack_base) {
                kfree(thread_table[i].stack_base);
            }
            
            // Clear thread entry
            thread_table[i].tid = 0;
            thread_table[i].state = THREAD_STATE_ZOMBIE;
            thread_table[i].entry_point = NULL;
            thread_table[i].stack_base = NULL;
            thread_table[i].stack_top = NULL;
        }
    }
}

// Print scheduler statistics
void sched_print_stats(void) {
    serial_puts("[SCHED] Scheduler Statistics:\r\n");
    
    for (int i = 0; i < MAX_THREADS; i++) {
        if (thread_table[i].tid != 0) {
            serial_puts("  Thread: ");
            serial_puts(thread_table[i].name);
            serial_puts(" State: ");
            switch (thread_table[i].state) {
                case THREAD_STATE_READY: serial_puts("READY"); break;
                case THREAD_STATE_RUNNING: serial_puts("RUNNING"); break;
                case THREAD_STATE_BLOCKED: serial_puts("BLOCKED"); break;
                case THREAD_STATE_ZOMBIE: serial_puts("ZOMBIE"); break;
            }
            serial_puts("\r\n");
        }
    }
}