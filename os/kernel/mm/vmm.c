#include <myria/types.h>
#include <myria/kapi.h>

// Forward declare real PMM functions
extern u64 pmm_alloc_page(void);
extern void pmm_free_page(u64 phys_addr);

// Page table manipulation
#define PML4_INDEX(vaddr)   (((vaddr) >> 39) & 0x1FF)
#define PDPT_INDEX(vaddr)   (((vaddr) >> 30) & 0x1FF)
#define PD_INDEX(vaddr)     (((vaddr) >> 21) & 0x1FF)
#define PT_INDEX(vaddr)     (((vaddr) >> 12) & 0x1FF)

// Page table entry flags
#define PAGE_PRESENT    (1UL << 0)
#define PAGE_WRITE      (1UL << 1)
#define PAGE_USER       (1UL << 2)
#define PAGE_PWT        (1UL << 3)
#define PAGE_PCD        (1UL << 4)
#define PAGE_ACCESSED   (1UL << 5)
#define PAGE_DIRTY      (1UL << 6)
#define PAGE_HUGE       (1UL << 7)  // 2MB pages
#define PAGE_GLOBAL     (1UL << 8)
#define PAGE_NX         (1UL << 63) // No execute

// Address manipulation
#define PAGE_ALIGN_DOWN(addr) ((addr) & ~(PAGE_SIZE - 1))
#define PAGE_ALIGN_UP(addr)   (((addr) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))
#define PTE_ADDR_MASK         0x000FFFFFFFFFF000UL

// Current page table (from initial paging setup)
static u64 *current_pml4 = NULL;

// Enhanced kernel heap management - use area within mapped kernel space  
// Place heap after kernel code/data sections
static u64 kernel_heap_start = KERNEL_VMA_BASE + 0x1000000; // 16MB offset from kernel base

// Forward declare serial functions
extern void serial_puts(const char *str);

// No forward declarations needed for simplified version

void vmm_init(void) {
    serial_puts("[VMM] Initializing enhanced virtual memory manager\r\n");
    
    // Get current CR3 register (PML4 address)
    u64 cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    
    // Convert physical PML4 address to virtual address
    current_pml4 = (u64 *)PHYS_TO_VIRT(cr3 & PTE_ADDR_MASK);
    
    serial_puts("[VMM] Page table pointer initialized\r\n");
    
    // For now, skip complex heap initialization to avoid PMM exhaustion
    // Use direct allocation within the existing mapped kernel space
    serial_puts("[VMM] Using simplified heap management\r\n");
    
    serial_puts("[VMM] Enhanced virtual memory manager initialized\r\n");
}


// Get physical address from page table entry
static u64 pte_to_phys(u64 pte) {
    return pte & PTE_ADDR_MASK;
}

// Create a new page table and return its physical address
static u64 alloc_page_table(void) {
    u64 phys = pmm_alloc_page();
    if (phys == 0) {
        return 0;
    }
    
    // Zero out the new page table
    u64 *table = (u64 *)(phys + KERNEL_VMA_BASE);
    for (int i = 0; i < 512; i++) {
        table[i] = 0;
    }
    
    return phys;
}

// Map a virtual address to a physical address
bool vmm_map_page(u64 vaddr, u64 paddr, u64 flags) {
    // Ensure addresses are page-aligned
    vaddr = PAGE_ALIGN_DOWN(vaddr);
    paddr = PAGE_ALIGN_DOWN(paddr);
    
    // Extract indices
    u64 pml4_idx = PML4_INDEX(vaddr);
    u64 pdpt_idx = PDPT_INDEX(vaddr);
    u64 pd_idx = PD_INDEX(vaddr);
    u64 pt_idx = PT_INDEX(vaddr);
    
    // Walk/create page table hierarchy
    u64 *pml4 = current_pml4;
    
    // Get/create PDPT
    if (!(pml4[pml4_idx] & PAGE_PRESENT)) {
        u64 pdpt_phys = alloc_page_table();
        if (pdpt_phys == 0) return false;
        pml4[pml4_idx] = pdpt_phys | PAGE_PRESENT | PAGE_WRITE;
    }
    u64 *pdpt = (u64 *)(pte_to_phys(pml4[pml4_idx]) + KERNEL_VMA_BASE);
    
    // Get/create PD
    if (!(pdpt[pdpt_idx] & PAGE_PRESENT)) {
        u64 pd_phys = alloc_page_table();
        if (pd_phys == 0) return false;
        pdpt[pdpt_idx] = pd_phys | PAGE_PRESENT | PAGE_WRITE;
    }
    u64 *pd = (u64 *)(pte_to_phys(pdpt[pdpt_idx]) + KERNEL_VMA_BASE);
    
    // Get/create PT
    if (!(pd[pd_idx] & PAGE_PRESENT)) {
        u64 pt_phys = alloc_page_table();
        if (pt_phys == 0) return false;
        pd[pd_idx] = pt_phys | PAGE_PRESENT | PAGE_WRITE;
    }
    u64 *pt = (u64 *)(pte_to_phys(pd[pd_idx]) + KERNEL_VMA_BASE);
    
    // Set page table entry
    pt[pt_idx] = paddr | flags | PAGE_PRESENT;
    
    // Flush TLB for this page
    __asm__ volatile("invlpg (%0)" : : "r"(vaddr) : "memory");
    
    return true;
}

// Unmap a virtual address
void vmm_unmap_page(u64 vaddr) {
    vaddr = PAGE_ALIGN_DOWN(vaddr);
    
    u64 pml4_idx = PML4_INDEX(vaddr);
    u64 pdpt_idx = PDPT_INDEX(vaddr);
    u64 pd_idx = PD_INDEX(vaddr);
    u64 pt_idx = PT_INDEX(vaddr);
    
    u64 *pml4 = current_pml4;
    
    // Walk page table hierarchy
    if (!(pml4[pml4_idx] & PAGE_PRESENT)) return;
    u64 *pdpt = (u64 *)(pte_to_phys(pml4[pml4_idx]) + KERNEL_VMA_BASE);
    
    if (!(pdpt[pdpt_idx] & PAGE_PRESENT)) return;
    u64 *pd = (u64 *)(pte_to_phys(pdpt[pdpt_idx]) + KERNEL_VMA_BASE);
    
    if (!(pd[pd_idx] & PAGE_PRESENT)) return;
    u64 *pt = (u64 *)(pte_to_phys(pd[pd_idx]) + KERNEL_VMA_BASE);
    
    // Clear page table entry
    pt[pt_idx] = 0;
    
    // Flush TLB
    __asm__ volatile("invlpg (%0)" : : "r"(vaddr) : "memory");
}

// Get physical address mapped to virtual address
u64 vmm_get_physical(u64 vaddr) {
    vaddr = PAGE_ALIGN_DOWN(vaddr);
    
    u64 pml4_idx = PML4_INDEX(vaddr);
    u64 pdpt_idx = PDPT_INDEX(vaddr);
    u64 pd_idx = PD_INDEX(vaddr);
    u64 pt_idx = PT_INDEX(vaddr);
    
    u64 *pml4 = current_pml4;
    
    // Walk page table hierarchy
    if (!(pml4[pml4_idx] & PAGE_PRESENT)) return 0;
    u64 *pdpt = (u64 *)(pte_to_phys(pml4[pml4_idx]) + KERNEL_VMA_BASE);
    
    if (!(pdpt[pdpt_idx] & PAGE_PRESENT)) return 0;
    u64 *pd = (u64 *)(pte_to_phys(pdpt[pdpt_idx]) + KERNEL_VMA_BASE);
    
    if (!(pd[pd_idx] & PAGE_PRESENT)) return 0;
    u64 *pt = (u64 *)(pte_to_phys(pd[pd_idx]) + KERNEL_VMA_BASE);
    
    if (!(pt[pt_idx] & PAGE_PRESENT)) return 0;
    
    return pte_to_phys(pt[pt_idx]);
}


// Simplified kernel heap allocator that works within mapped space
void *kmalloc(u64 size) {
    if (size == 0) return NULL;
    
    // Align size to 8 bytes 
    size = ALIGN_UP(size, 8);
    
    // Simple allocation within kernel space - use a basic bump allocator
    // This avoids the complexity of heap management for now
    static u64 simple_heap_ptr = 0;
    
    // Initialize heap pointer on first use
    if (simple_heap_ptr == 0) {
        simple_heap_ptr = kernel_heap_start;
    }
    
    // Check if we have enough space (within existing mapped kernel memory)
    if (simple_heap_ptr + size >= kernel_heap_start + (1024 * 1024)) { // 1MB limit
        serial_puts("[VMM] kmalloc: Simple heap exhausted!\r\n");
        return NULL;
    }
    
    void *result = (void *)simple_heap_ptr;
    simple_heap_ptr += size;
    
    return result;
}

// Free kernel heap memory (simplified - no-op for bump allocator)
void kfree(void *ptr) {
    if (ptr == NULL) return;
    
    // For the simple bump allocator, we can't really free individual blocks
    // In a full implementation, we would track free blocks and reuse them
    // For now, this is just a no-op
}

// Map multiple pages
bool vmm_map_pages(u64 vaddr_start, u64 paddr_start, u64 count, u64 flags) {
    for (u64 i = 0; i < count; i++) {
        u64 vaddr = vaddr_start + i * PAGE_SIZE;
        u64 paddr = paddr_start + i * PAGE_SIZE;
        
        if (!vmm_map_page(vaddr, paddr, flags)) {
            // Clean up partial mapping
            for (u64 j = 0; j < i; j++) {
                vmm_unmap_page(vaddr_start + j * PAGE_SIZE);
            }
            return false;
        }
    }
    return true;
}

// Unmap multiple pages
void vmm_unmap_pages(u64 vaddr_start, u64 count) {
    for (u64 i = 0; i < count; i++) {
        vmm_unmap_page(vaddr_start + i * PAGE_SIZE);
    }
}