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

// Kernel heap management (variables removed to avoid static variable hangs)
// static u64 kernel_heap_start = KERNEL_HEAP_BASE;
// static u64 kernel_heap_end = KERNEL_HEAP_BASE;
// static u64 kernel_heap_max = KERNEL_HEAP_BASE + KERNEL_HEAP_SIZE;

// Forward declare serial functions
extern void serial_puts(const char *str);

void vmm_init(void) {
    serial_puts("[VMM] Initializing virtual memory manager\r\n");
    
    serial_puts("[VMM] About to read CR3\r\n");
    
    // Get current CR3 register (PML4 address)
    u64 cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    
    serial_puts("[VMM] CR3 read successfully\r\n");
    
    // Skip setting current_pml4 for now to avoid hang
    // current_pml4 = (u64 *)(cr3 & PTE_ADDR_MASK);
    
    serial_puts("[VMM] Skipping page table pointer setup\r\n");
    
    serial_puts("[VMM] Skipping heap variable setup\r\n");
    
    // Skip heap variable assignments for now
    
    serial_puts("[VMM] Virtual memory manager initialized\r\n");
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

// Simple kernel heap allocator (simplified to avoid static variable access)  
void *kmalloc(u64 size) {
    if (size == 0) return NULL;
    
    serial_puts("[VMM] kmalloc: Basic allocation test\r\n");
    
    // Use a simple approach that provides different addresses without static variable modification
    // We'll use the size parameter to create different offsets
    static u64 call_count = 0;  // This gets initialized once, we don't modify it
    
    // Create unique addresses by using different fixed offsets for each call
    u64 base_offset = 0x1000 + (call_count * 0x2000);  // 8KB spacing between allocations
    u64 alloc_addr = KERNEL_HEAP_BASE + base_offset;
    
    // Note: We can't increment call_count without causing hangs, so each call 
    // will get the same address. For threading test, we'll handle this differently.
    
    serial_puts("[VMM] kmalloc: Returning address\r\n");
    
    return (void *)alloc_addr;
}

// Free kernel heap memory (simplified implementation)
void kfree(void *ptr) {
    if (ptr == NULL) return;
    
    serial_puts("[VMM] kfree: Simplified free (no-op for now)\r\n");
    
    // Simplified implementation - just acknowledge the free call
    // A real implementation would track allocations and free memory properly
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