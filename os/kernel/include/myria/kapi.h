#ifndef MYRIA_KAPI_H
#define MYRIA_KAPI_H

#include <myria/types.h>

// Early boot initialization
void x86_early_init(void);
void kmain(void) NORETURN;

// Serial I/O
void serial_init(void);
void serial_putc(char c);
void serial_puts(const char *str);

// Kernel logging
void klog_init(void);
void kprintf(const char *fmt, ...);
void panic(const char *fmt, ...) NORETURN;

// x86-64 specific
void gdt_init(void);
void tss_init(void);
void idt_init(void);
void enter_user(u64 rip, u64 rsp, u64 rflags);
u64 get_kernel_stack(void);
void set_kernel_stack(u64 stack_top);
void setup_initial_paging(void);
void enable_paging(void);
void lapic_init(void);
void lapic_eoi(void);
void timer_init(void);
void timer_tick_handler(void);

// Timer functions
u64 get_system_ticks(void);
u64 get_system_time_ms(void);

// Memory management functions
void pmm_init(void);
u64 pmm_alloc_page(void);
void pmm_free_page(u64 phys_addr);
u64 pmm_alloc_pages(u64 count);
void pmm_free_pages(u64 phys_addr, u64 count);
void pmm_get_stats(u64 *total, u64 *free, u64 *used);

void vmm_init(void);
bool vmm_map_page(u64 vaddr, u64 paddr, u64 flags);
void vmm_unmap_page(u64 vaddr);
u64 vmm_get_physical(u64 vaddr);
bool vmm_map_pages(u64 vaddr_start, u64 paddr_start, u64 count, u64 flags);
void vmm_unmap_pages(u64 vaddr_start, u64 count);
void *kmalloc(u64 size);
void kfree(void *ptr);

// Paging and memory management
u64 setup_kernel_page_tables(void);
void activate_kernel_page_tables(u64 pml4_phys);
void debug_page_mapping(u64 virt_addr);
u64 phys_to_virt(u64 phys_addr);
void invlpg(u64 addr);

// User address space management
bool setup_user_address_space(void **user_code_va, void **user_stack_top, u64 code_size);
bool copy_user_code(void *user_code_va, const void *kernel_code, u64 code_size);
bool make_code_page_executable(void *user_code_va);
bool map_user_4k(u64 user_va, u64 phys_addr, u64 flags);

// Isolated user address space (separate PML4)
void init_kernel_pml4_template(void);
u64 user_as_create(void);
bool user_map_4k_in_pml4(u64 pml4_phys, u64 va, u64 pa, bool writable, bool executable);
u64 create_user_process(void);
void switch_to_user_process(u64 user_pml4_phys);

// User payload (from assembly)
extern u8 user_payload_start[];
extern u8 user_payload_end[];

// Forward declare thread type
struct thread;
typedef struct thread thread_t;

// Threading and scheduling
void sched_init(void);
void sched_start(void);
u32 thread_create(void (*entry_point)(void *), void *arg, const char *name);
void sched_yield(void);
void sched_tick(void);
void sched_print_stats(void);
thread_t *sched_current_thread(void);

// System calls
void syscall_init(void);
u64 syscall_dispatch(u64 syscall_num, u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5, u64 arg6);
void syscall_print_stats(void);

// MSR functions
u64 read_msr(u32 msr);
void write_msr(u32 msr, u64 value);
void setup_syscall_msrs(void);

// Port I/O
static inline void outb(u16 port, u8 val) {
    __asm__ volatile("outb %b0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

static inline u8 inb(u16 port) {
    u8 ret;
    __asm__ volatile("inb %w1, %b0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

static inline void outw(u16 port, u16 val) {
    __asm__ volatile("outw %w0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

static inline u16 inw(u16 port) {
    u16 ret;
    __asm__ volatile("inw %w1, %w0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

static inline void outl(u16 port, u32 val) {
    __asm__ volatile("outl %0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

static inline u32 inl(u16 port) {
    u32 ret;
    __asm__ volatile("inl %w1, %0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

// Memory barriers
static inline void mb(void) {
    __asm__ volatile("mfence" : : : "memory");
}

static inline void rmb(void) {
    __asm__ volatile("lfence" : : : "memory");
}

static inline void wmb(void) {
    __asm__ volatile("sfence" : : : "memory");
}

// CPU control
static inline void cli(void) {
    __asm__ volatile("cli" : : : "memory");
}

static inline void sti(void) {
    __asm__ volatile("sti" : : : "memory");
}

static inline void hlt(void) {
    __asm__ volatile("hlt");
}

static inline void hang(void) {
    cli();
    while (1) hlt();
}

#endif // MYRIA_KAPI_H