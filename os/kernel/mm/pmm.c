#include <myria/types.h>
#include <myria/kapi.h>

// Limine memory map request - declared in start.S
extern struct limine_memmap_request limine_memmap_request;

// Physical memory state
static u64 total_memory = 0;
static u64 free_memory = 0;
static u64 used_memory = 0;

// Simple bitmap-based page frame allocator
#define MAX_PAGES (8 * 1024)   // Support up to 32MB of RAM for early boot (8K * 4KB pages)
static u8 page_bitmap[MAX_PAGES / 8];  // 1 bit per page
static u64 bitmap_pages = 0;
static u64 first_free_page = 0;

// Kernel memory boundaries (from linker script)
extern char __kernel_start[];
extern char __kernel_end[];

static inline void set_page_used(u64 page_idx) {
    u64 byte_idx = page_idx / 8;
    u64 bit_idx = page_idx % 8;
    page_bitmap[byte_idx] |= (1 << bit_idx);
}

static inline void set_page_free(u64 page_idx) {
    u64 byte_idx = page_idx / 8;
    u64 bit_idx = page_idx % 8;
    page_bitmap[byte_idx] &= ~(1 << bit_idx);
}

static inline bool is_page_free(u64 page_idx) {
    u64 byte_idx = page_idx / 8;
    u64 bit_idx = page_idx % 8;
    return !(page_bitmap[byte_idx] & (1 << bit_idx));
}

// Forward declare serial functions
extern void serial_puts(const char *str);

void pmm_init(void) {
    serial_puts("[PMM] Starting PMM initialization\r\n");
    
    // Completely skip Limine memory map for now - use hardcoded values
    total_memory = 512 * 1024 * 1024; // 512MB
    free_memory = 256 * 1024 * 1024;  // 256MB free  
    used_memory = total_memory - free_memory;
    bitmap_pages = MAX_PAGES;
    
    serial_puts("[PMM] Using hardcoded memory configuration\r\n");
    serial_puts("[PMM] PMM initialization complete\r\n");
}

u64 pmm_alloc_page(void) {
    // Simple first-fit allocation
    for (u64 page = first_free_page; page < bitmap_pages; page++) {
        if (is_page_free(page)) {
            set_page_used(page);
            free_memory -= PAGE_SIZE;
            used_memory += PAGE_SIZE;
            
            // Optimization: start next search from this position
            if (page == first_free_page) {
                first_free_page++;
            }
            
            return page * PAGE_SIZE;
        }
    }
    
    // No free pages found
    return 0;
}

void pmm_free_page(u64 phys_addr) {
    if (phys_addr == 0 || phys_addr % PAGE_SIZE != 0) {
        return;
    }
    
    u64 page = phys_addr / PAGE_SIZE;
    if (page >= bitmap_pages) {
        return;
    }
    
    if (is_page_free(page)) {
        // Already free, might be double-free bug
        return;
    }
    
    set_page_free(page);
    free_memory += PAGE_SIZE;
    used_memory -= PAGE_SIZE;
    
    // Optimization: update first free page hint
    if (page < first_free_page) {
        first_free_page = page;
    }
}

// Allocate multiple contiguous pages
u64 pmm_alloc_pages(u64 count) {
    if (count == 1) {
        return pmm_alloc_page();
    }
    
    // Find contiguous free pages
    for (u64 start_page = first_free_page; start_page <= bitmap_pages - count; start_page++) {
        bool found_region = true;
        
        // Check if all pages in region are free
        for (u64 i = 0; i < count; i++) {
            if (!is_page_free(start_page + i)) {
                found_region = false;
                break;
            }
        }
        
        if (found_region) {
            // Allocate all pages in region
            for (u64 i = 0; i < count; i++) {
                set_page_used(start_page + i);
            }
            
            free_memory -= count * PAGE_SIZE;
            used_memory += count * PAGE_SIZE;
            
            return start_page * PAGE_SIZE;
        }
    }
    
    return 0;  // No contiguous region found
}

void pmm_free_pages(u64 phys_addr, u64 count) {
    for (u64 i = 0; i < count; i++) {
        pmm_free_page(phys_addr + i * PAGE_SIZE);
    }
}

// Get memory statistics
void pmm_get_stats(u64 *total, u64 *free, u64 *used) {
    if (total) *total = total_memory;
    if (free) *free = free_memory;
    if (used) *used = used_memory;
}