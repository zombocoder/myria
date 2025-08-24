#include <myria/types.h>
#include <myria/kapi.h>

// No external declarations, no Limine access
static u64 total_memory = 0;

void pmm_init(void) {
    kprintf("[PMM] TEST: Starting PMM initialization\n");
    
    // Just set some basic values without accessing Limine
    total_memory = 512 * 1024 * 1024;  // Assume 512MB for test
    
    kprintf("[PMM] TEST: Set total memory to %lu MB\n", total_memory / (1024 * 1024));
    kprintf("[PMM] TEST: PMM initialization completed successfully\n");
}

// Stub functions
u64 pmm_alloc_page(void) {
    return 0;
}

void pmm_free_page(u64 phys_addr UNUSED) {
}

u64 pmm_alloc_pages(u64 count UNUSED) {
    return 0;
}

void pmm_free_pages(u64 phys_addr UNUSED, u64 count UNUSED) {
}

void pmm_get_stats(u64 *total, u64 *free UNUSED, u64 *used UNUSED) {
    if (total) *total = total_memory;
}