#include <myria/types.h>
#include <myria/kapi.h>

// Forward declare serial functions
extern void serial_puts(const char *str);

// Thread states (simplified)
typedef enum {
    THREAD_STATE_READY = 0,
    THREAD_STATE_RUNNING,
    THREAD_STATE_BLOCKED,
    THREAD_STATE_ZOMBIE
} thread_state_t;

// Simplified Thread Control Block
typedef struct {
    u32 tid;                    
    thread_state_t state;
    u8 priority;
    char name[32];
    void (*entry_point)(void*);
    void *arg;
    void *stack;
} thread_t;

// Basic scheduler state  
#define MAX_THREADS 4
static thread_t thread_table[MAX_THREADS];
static thread_t *current_thread = NULL;
// static u32 next_tid = 1;        // Unused - removed to avoid static variable issues
// static u32 thread_count = 0;    // Unused - removed to avoid static variable issues

// Initialize the basic threading system
void sched_init(void) {
    serial_puts("[SCHED] Initializing basic scheduler\r\n");
    
    serial_puts("[SCHED] Skipping static variable assignments to avoid hang\r\n");
    
    // Skip static variable assignments for now - they cause hangs
    // current_thread = NULL;  // Already initialized statically
    // thread_count = 0;       // Already initialized statically  
    // next_tid = 1;           // Already initialized statically
    
    serial_puts("[SCHED] Basic scheduler initialized\r\n");
}

// Get current thread (stub)
thread_t *sched_current_thread(void) {
    return current_thread;
}

// Basic yield function (stub)
void sched_yield(void) {
    // No-op for now
}

// Basic tick function (stub) 
void sched_tick(void) {
    // No-op for now
}

// Create a simple kernel thread (without full context switching)
u32 thread_create(void (*entry_point)(void *), void *arg, const char *name) {
    serial_puts("[SCHED] thread_create: Starting thread creation\r\n");
    
    // Can't check thread_count due to static variable issues, just find free slot
    
    serial_puts("[SCHED] thread_create: Looking for free slot\r\n");
    
    // Find free slot
    int slot = -1;
    for (int i = 0; i < MAX_THREADS; i++) {
        if (thread_table[i].tid == 0) {
            slot = i;
            break;
        }
    }
    
    serial_puts("[SCHED] thread_create: Finished slot search\r\n");
    
    if (slot == -1) {
        serial_puts("[SCHED] No free thread slot\r\n");
        return 0;
    }
    
    serial_puts("[SCHED] thread_create: Getting pre-allocated stack\r\n");
    
    // Use pre-allocated stack space to avoid kmalloc (which can't provide unique addresses)
    // Static arrays are initialized once and don't require assignments that cause hangs
    static u8 thread_stack_0[4096] __attribute__((aligned(16)));
    static u8 thread_stack_1[4096] __attribute__((aligned(16)));
    static u8 thread_stack_2[4096] __attribute__((aligned(16)));
    static u8 thread_stack_3[4096] __attribute__((aligned(16)));
    
    void *stack = NULL;
    if (slot == 0) stack = thread_stack_0;
    else if (slot == 1) stack = thread_stack_1; 
    else if (slot == 2) stack = thread_stack_2;
    else if (slot == 3) stack = thread_stack_3;
    
    if (!stack) {
        serial_puts("[SCHED] No pre-allocated stack available\r\n");
        return 0;
    }
    
    serial_puts("[SCHED] thread_create: About to initialize thread structure\r\n");
    
    // Initialize thread (avoid static variable modifications that cause hangs)
    thread_table[slot].tid = slot + 1;  // Simple TID based on slot number
    thread_table[slot].state = THREAD_STATE_READY;
    thread_table[slot].priority = 1;
    thread_table[slot].entry_point = entry_point;
    thread_table[slot].arg = arg;
    thread_table[slot].stack = stack;
    
    // Set name (avoid static variable modification)
    for (int i = 0; i < 32; i++) thread_table[slot].name[i] = 0;
    if (name) {
        for (int i = 0; name[i] && i < 31; i++) {
            thread_table[slot].name[i] = name[i];
        }
    }
    
    // Can't increment thread_count due to static variable hang issue
    
    serial_puts("[SCHED] Created thread: ");
    serial_puts(thread_table[slot].name);
    serial_puts("\r\n");
    
    return thread_table[slot].tid;
}

// Run threads cooperatively (simplified scheduler)
void sched_run_threads(void) {
    serial_puts("[SCHED] Running threads cooperatively\r\n");
    
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