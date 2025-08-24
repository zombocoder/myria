#include <myria/types.h>
#include <myria/kapi.h>

// Limine memory map request - declared in start.S
extern struct limine_memmap_request limine_memmap_request;

// Physical memory state (no large static arrays)
static u64 total_memory = 0;
static u64 free_memory = 0;
static u64 used_memory = 0;

// Kernel memory boundaries (from linker script)
extern char __kernel_start[];
extern char __kernel_end[];

void pmm_init(void) {
    kprintf("[PMM] Initializing physical memory manager\n");
    
    // Check if Limine provided memory map
    if (!limine_memmap_request.response) {
        panic("[PMM] No memory map provided by bootloader");
        return;
    }
    
    struct limine_memmap_response *memmap = limine_memmap_request.response;
    kprintf("[PMM] Memory map has %u entries\n", memmap->entry_count);
    
    // Calculate kernel physical range
    u64 kernel_phys_start = (u64)__kernel_start - KERNEL_VMA_BASE;
    u64 kernel_phys_end = (u64)__kernel_end - KERNEL_VMA_BASE;
    
    kprintf("[PMM] Kernel physical range: 0x%016lx - 0x%016lx\n", 
            kernel_phys_start, kernel_phys_end);
    
    // Process memory map entries (just count memory for now)
    for (u64 i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        
        const char *type_str = "UNKNOWN";
        switch (entry->type) {
            case LIMINE_MEMMAP_USABLE:
                type_str = "USABLE";
                free_memory += entry->length;
                break;
            case LIMINE_MEMMAP_RESERVED:
                type_str = "RESERVED";
                break;
            case LIMINE_MEMMAP_KERNEL_AND_MODULES:
                type_str = "KERNEL_AND_MODULES";
                break;
        }
        
        kprintf("[PMM] Entry %u: 0x%016lx - 0x%016lx (%s, %lu KB)\n",
                i, entry->base, entry->base + entry->length - 1,
                type_str, entry->length / 1024);
        
        total_memory += entry->length;
    }
    
    used_memory = total_memory - free_memory;
    
    kprintf("[PMM] Total memory: %lu MB\n", total_memory / (1024 * 1024));
    kprintf("[PMM] Free memory: %lu MB\n", free_memory / (1024 * 1024));
    kprintf("[PMM] Used memory: %lu MB\n", used_memory / (1024 * 1024));
    kprintf("[PMM] Physical memory manager initialized\n");
}

// Stub functions for minimal test
u64 pmm_alloc_page(void) {
    return 0;  // Not implemented in minimal version
}

void pmm_free_page(u64 phys_addr UNUSED) {
    // Not implemented in minimal version
}

u64 pmm_alloc_pages(u64 count UNUSED) {
    return 0;  // Not implemented in minimal version
}

void pmm_free_pages(u64 phys_addr UNUSED, u64 count UNUSED) {
    // Not implemented in minimal version
}

void pmm_get_stats(u64 *total, u64 *free, u64 *used) {
    if (total) *total = total_memory;
    if (free) *free = free_memory;
    if (used) *used = used_memory;
}