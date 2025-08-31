#include <myria/types.h>
#include <myria/kapi.h>

// Forward declare serial functions
extern void serial_puts(const char *str);

// Enhanced PMM with free page tracking
#define MAX_FREE_PAGES 1024
#define PMM_START_ADDR 0x200000  // Start at 2MB
#define PMM_END_ADDR   0x40000000 // End at 1GB for now

// PMM state - BSS will be properly cleared by paging.c before we access these
static u64 free_pages[MAX_FREE_PAGES];  // BSS array, cleared by proper page table setup
static u32 free_page_count;             // BSS variable, cleared by proper page table setup
static u64 next_page_addr;              // BSS variable, cleared by proper page table setup
static u64 total_pages;                 // BSS variable, cleared by proper page table setup
static u64 allocated_pages;             // BSS variable, cleared by proper page table setup
static u64 freed_pages;                 // BSS variable, cleared by proper page table setup

// Initialize available memory pool
static void init_memory_pool(void) {
    serial_puts("[PMM] Initializing memory pool\r\n");
    
    // Set up the memory pool calculations (BSS is now cleared)
    total_pages = (PMM_END_ADDR - PMM_START_ADDR) / PAGE_SIZE;
    free_page_count = 0;
    next_page_addr = PMM_START_ADDR;
    
    serial_puts("[PMM] Memory pool initialized\r\n");
}

void pmm_init(void) {
    serial_puts("[PMM] Starting enhanced PMM (BSS properly cleared)\r\n");
    
    // Test that we can now safely access BSS variables
    serial_puts("[PMM] Testing BSS variable access...\r\n");
    free_page_count = 0;  // Should work now!
    serial_puts("[PMM] BSS variables accessible!\r\n");
    
    // Reset all counters (they should already be 0 from BSS clearing)
    allocated_pages = 0;
    freed_pages = 0;
    
    serial_puts("[PMM] About to initialize memory pool\r\n");
    // Initialize the memory pool
    init_memory_pool();
    
    serial_puts("[PMM] Enhanced PMM ready\r\n");
}

u64 pmm_alloc_page(void) {
    // First try to get a page from the free stack
    if (free_page_count > 0) {
        free_page_count--;
        allocated_pages++;
        return free_pages[free_page_count];
    }
    
    // If no free pages, allocate a new one
    if (next_page_addr < PMM_END_ADDR) {
        u64 addr = next_page_addr;
        next_page_addr += PAGE_SIZE;
        allocated_pages++;
        return addr;
    }
    
    // Out of memory
    serial_puts("[PMM] ERROR: Out of memory!\r\n");
    return 0;
}

void pmm_free_page(u64 phys_addr) {
    // Validate address
    if (phys_addr < PMM_START_ADDR || phys_addr >= PMM_END_ADDR) {
        return; // Invalid address
    }
    
    // Check if we have space in the free stack
    if (free_page_count < MAX_FREE_PAGES) {
        free_pages[free_page_count] = phys_addr;
        free_page_count++;
        freed_pages++;
        
        if (allocated_pages > 0) {
            allocated_pages--;
        }
    }
}

u64 pmm_alloc_pages(u64 count) {
    if (count == 0) return 0;
    
    // For multiple pages, we need contiguous allocation
    if (next_page_addr + (count * PAGE_SIZE) <= PMM_END_ADDR) {
        u64 addr = next_page_addr;
        next_page_addr += count * PAGE_SIZE;
        allocated_pages += count;
        return addr;
    }
    
    return 0; // Can't allocate contiguous pages
}

void pmm_free_pages(u64 phys_addr, u64 count) {
    // Free individual pages
    for (u64 i = 0; i < count; i++) {
        pmm_free_page(phys_addr + i * PAGE_SIZE);
    }
}

void pmm_get_stats(u64 *total, u64 *free, u64 *used) {
    if (total) *total = total_pages;
    if (free) *free = free_page_count + ((PMM_END_ADDR - next_page_addr) / PAGE_SIZE);
    if (used) *used = allocated_pages;
}