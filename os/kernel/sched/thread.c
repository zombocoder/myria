#include <myria/types.h>
#include <myria/kapi.h>

// Thread states
typedef enum {
    THREAD_STATE_READY = 0,
    THREAD_STATE_RUNNING,
    THREAD_STATE_BLOCKED,
    THREAD_STATE_ZOMBIE,
    THREAD_STATE_TERMINATED
} thread_state_t;

// Thread priorities
#define THREAD_PRIORITY_IDLE    0
#define THREAD_PRIORITY_LOW     1
#define THREAD_PRIORITY_NORMAL  2
#define THREAD_PRIORITY_HIGH    3
#define THREAD_PRIORITY_REAL    4

// CPU context saved during context switch
typedef struct {
    // General purpose registers
    u64 rax, rbx, rcx, rdx;
    u64 rsi, rdi, rbp, rsp;
    u64 r8, r9, r10, r11;
    u64 r12, r13, r14, r15;
    
    // Control registers
    u64 rip;
    u64 rflags;
    
    // Segment registers (minimal in x86-64)
    u16 cs, ss;
    
    // FPU/SSE state pointer (allocated separately)
    void *fpu_state;
} cpu_context_t;

// Thread Control Block (TCB)
typedef struct thread {
    // Thread identification
    u32 tid;                    // Thread ID
    u32 pid;                    // Process ID (for future use)
    
    // Thread state
    thread_state_t state;
    u8 priority;
    u64 time_slice;             // Remaining time slice in ticks
    u64 total_runtime;          // Total execution time
    
    // CPU context
    cpu_context_t context;
    
    // Stack information
    void *stack_base;           // Stack base address
    u64 stack_size;             // Stack size
    void *kernel_stack;         // Kernel stack for this thread
    
    // Scheduling data
    struct thread *next;        // Next thread in ready queue
    struct thread *prev;        // Previous thread in ready queue
    u64 last_scheduled;         // Last time thread was scheduled
    
    // Thread name (for debugging)
    char name[32];
} thread_t;

// Scheduler state
static thread_t *current_thread = NULL;
static thread_t *idle_thread = NULL;
static thread_t *ready_queue_head = NULL;
static thread_t *ready_queue_tail = NULL;
static u32 next_tid = 1;
static u64 scheduler_ticks = 0;

// Thread list (simple array for now)
#define MAX_THREADS 8  // Reduce for early boot - will expand later
static thread_t *thread_table[MAX_THREADS];
static u32 thread_count = 0;

// Default time slice (in timer ticks)
#define DEFAULT_TIME_SLICE 10

// Forward declarations
extern void context_switch(cpu_context_t *old_ctx, cpu_context_t *new_ctx);
extern void thread_entry_trampoline(void);

// Forward declare serial functions
extern void serial_puts(const char *str);

// Initialize the threading system
void sched_init(void) {
    serial_puts("[SCHED] Initializing basic scheduler\r\n");
    
    // Clear thread table
    for (int i = 0; i < MAX_THREADS; i++) {
        thread_table[i] = NULL;
    }
    
    // For now, just set up basic state without creating actual threads
    current_thread = NULL;
    idle_thread = NULL;
    ready_queue_head = NULL;
    ready_queue_tail = NULL;
    thread_count = 0;
    
    serial_puts("[SCHED] Basic scheduler initialized\r\n");
}

// Add thread to ready queue
static void add_to_ready_queue(thread_t *thread) {
    thread->state = THREAD_STATE_READY;
    thread->next = NULL;
    thread->prev = ready_queue_tail;
    
    if (ready_queue_tail) {
        ready_queue_tail->next = thread;
    } else {
        ready_queue_head = thread;
    }
    ready_queue_tail = thread;
}

// Remove thread from ready queue
static void remove_from_ready_queue(thread_t *thread) {
    if (thread->prev) {
        thread->prev->next = thread->next;
    } else {
        ready_queue_head = thread->next;
    }
    
    if (thread->next) {
        thread->next->prev = thread->prev;
    } else {
        ready_queue_tail = thread->prev;
    }
    
    thread->next = NULL;
    thread->prev = NULL;
}

// Get next thread from ready queue
static thread_t *get_next_thread(void) {
    if (ready_queue_head) {
        thread_t *thread = ready_queue_head;
        remove_from_ready_queue(thread);
        return thread;
    }
    return idle_thread;  // No threads ready, return idle
}

// Thread entry point wrapper
void thread_wrapper(void (*entry_point)(void *), void *arg) {
    // Call the actual thread function
    entry_point(arg);
    
    // Thread finished, mark as zombie
    current_thread->state = THREAD_STATE_ZOMBIE;
    
    // Schedule next thread
    sched_yield();
    
    // Should never reach here
    panic("[SCHED] Zombie thread continued execution");
}

// Create a new kernel thread
u32 thread_create(void (*entry_point)(void *), void *arg, const char *name) {
    if (thread_count >= MAX_THREADS) {
        kprintf("[SCHED] Thread table full\n");
        return 0;
    }
    
    // Allocate thread structure
    thread_t *thread = kmalloc(sizeof(thread_t));
    if (!thread) {
        kprintf("[SCHED] Failed to allocate thread structure\n");
        return 0;
    }
    
    // Allocate stack (8KB for now)
    const u64 stack_size = 8 * 1024;
    void *stack = kmalloc(stack_size);
    if (!stack) {
        kfree(thread);
        kprintf("[SCHED] Failed to allocate thread stack\n");
        return 0;
    }
    
    // Initialize thread structure
    thread->tid = next_tid++;
    thread->pid = 0;  // Kernel threads have PID 0
    thread->state = THREAD_STATE_READY;
    thread->priority = THREAD_PRIORITY_NORMAL;
    thread->time_slice = DEFAULT_TIME_SLICE;
    thread->total_runtime = 0;
    thread->stack_base = stack;
    thread->stack_size = stack_size;
    thread->kernel_stack = stack;
    thread->next = NULL;
    thread->prev = NULL;
    thread->last_scheduled = 0;
    
    // Set thread name
    for (int i = 0; i < 32; i++) thread->name[i] = 0;
    if (name) {
        for (int i = 0; name[i] && i < 31; i++) {
            thread->name[i] = name[i];
        }
    }
    
    // Initialize CPU context
    thread->context.rax = 0;
    thread->context.rbx = 0;
    thread->context.rcx = 0;
    thread->context.rdx = 0;
    thread->context.rsi = (u64)arg;         // Argument in RSI
    thread->context.rdi = (u64)entry_point; // Entry point in RDI
    thread->context.rbp = 0;
    thread->context.rsp = (u64)stack + stack_size - 8; // Stack pointer at top
    thread->context.r8 = 0;
    thread->context.r9 = 0;
    thread->context.r10 = 0;
    thread->context.r11 = 0;
    thread->context.r12 = 0;
    thread->context.r13 = 0;
    thread->context.r14 = 0;
    thread->context.r15 = 0;
    thread->context.rip = (u64)thread_wrapper;
    thread->context.rflags = 0x202; // IF=1, reserved bit=1
    thread->context.cs = 0x08;      // Kernel code segment
    thread->context.ss = 0x10;      // Kernel data segment
    thread->context.fpu_state = NULL;
    
    // Add to thread table
    for (int i = 1; i < MAX_THREADS; i++) {
        if (thread_table[i] == NULL) {
            thread_table[i] = thread;
            thread_count++;
            break;
        }
    }
    
    // Add to ready queue
    add_to_ready_queue(thread);
    
    kprintf("[SCHED] Created thread %u ('%s')\n", thread->tid, thread->name);
    return thread->tid;
}

// Yield CPU to next thread
void sched_yield(void) {
    cli();  // Disable interrupts during scheduling
    
    thread_t *old_thread = current_thread;
    thread_t *new_thread = get_next_thread();
    
    if (new_thread == old_thread) {
        sti();  // Re-enable interrupts
        return; // No other threads to run
    }
    
    // Update thread states
    if (old_thread->state == THREAD_STATE_RUNNING) {
        if (old_thread != idle_thread) {
            add_to_ready_queue(old_thread);
        }
    }
    
    new_thread->state = THREAD_STATE_RUNNING;
    new_thread->time_slice = DEFAULT_TIME_SLICE;
    new_thread->last_scheduled = scheduler_ticks;
    current_thread = new_thread;
    
    // Perform context switch
    context_switch(&old_thread->context, &new_thread->context);
    
    sti();  // Re-enable interrupts
}

// Timer-driven preemptive scheduling
void sched_tick(void) {
    scheduler_ticks++;
    
    if (current_thread) {
        current_thread->total_runtime++;
        
        if (current_thread->time_slice > 0) {
            current_thread->time_slice--;
        }
        
        // Preempt if time slice expired and there are other threads
        if (current_thread->time_slice == 0 && ready_queue_head != NULL) {
            sched_yield();
        }
    }
}

// Get current thread
thread_t *sched_current_thread(void) {
    return current_thread;
}

// Print scheduler statistics
void sched_print_stats(void) {
    kprintf("[SCHED] Scheduler Statistics:\n");
    kprintf("[SCHED] Total threads: %u\n", thread_count);
    kprintf("[SCHED] Scheduler ticks: %lu\n", scheduler_ticks);
    kprintf("[SCHED] Current thread: %u ('%s')\n", 
            current_thread ? current_thread->tid : 0,
            current_thread ? current_thread->name : "none");
    
    kprintf("[SCHED] Thread list:\n");
    for (int i = 0; i < MAX_THREADS; i++) {
        thread_t *t = thread_table[i];
        if (t) {
            const char *state_str = "UNKNOWN";
            switch (t->state) {
                case THREAD_STATE_READY: state_str = "READY"; break;
                case THREAD_STATE_RUNNING: state_str = "RUNNING"; break;
                case THREAD_STATE_BLOCKED: state_str = "BLOCKED"; break;
                case THREAD_STATE_ZOMBIE: state_str = "ZOMBIE"; break;
                case THREAD_STATE_TERMINATED: state_str = "TERMINATED"; break;
            }
            kprintf("[SCHED]   TID %u: '%s' (%s, runtime=%lu)\n", 
                    t->tid, t->name, state_str, t->total_runtime);
        }
    }
}