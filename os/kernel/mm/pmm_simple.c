#include <myria/types.h>
#include <myria/kapi.h>

// Forward declare serial functions
extern void serial_puts(const char *str);

// Simple counters - no complex data structures
static u64 next_page_addr = 0x200000; // Start allocating from 2MB

void pmm_init(void) {
    serial_puts("[PMM] Starting simple PMM\r\n");
    serial_puts("[PMM] PMM ready\r\n");
}

u64 pmm_alloc_page(void) {
    u64 addr = next_page_addr;
    next_page_addr += 4096; // Next page
    return addr;
}

void pmm_free_page(u64 phys_addr) {
    // No-op for simple allocator
    (void)phys_addr;
}