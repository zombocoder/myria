#include <myria/types.h>
#include <myria/kapi.h>

// Page flags - match our existing paging.c definitions
#define PTE_PRESENT     (1ULL << 0)
#define PTE_WRITABLE    (1ULL << 1)
#define PTE_USER        (1ULL << 2)
#define PTE_WRITETHROUGH (1ULL << 3)
#define PTE_NOCACHE     (1ULL << 4)
#define PTE_ACCESSED    (1ULL << 5)
#define PTE_DIRTY       (1ULL << 6)
#define PTE_HUGEPAGE    (1ULL << 7)
#define PTE_GLOBAL      (1ULL << 8)
#define PTE_NOEXECUTE   (1ULL << 63)
#define PTE_ADDR_MASK   0x000FFFFFFFFFF000ULL

// User-friendly flags
#define USER_PAGE_READ     (PTE_PRESENT | PTE_USER)
#define USER_PAGE_WRITE    (PTE_PRESENT | PTE_USER | PTE_WRITABLE)
#define USER_PAGE_EXEC     (PTE_PRESENT | PTE_USER)
#define USER_PAGE_RW_NOEXEC (PTE_PRESENT | PTE_USER | PTE_WRITABLE | PTE_NOEXECUTE)

// Functions from paging.c are now in kapi.h

// Helper functions
static inline u64 read_cr3(void) {
    u64 val;
    __asm__ volatile("mov %%cr3, %0" : "=r"(val));
    return val;
}

static inline u64 pml4_index(u64 va) { return (va >> 39) & 0x1FF; }
static inline u64 pdpt_index(u64 va) { return (va >> 30) & 0x1FF; }
static inline u64 pd_index(u64 va)   { return (va >> 21) & 0x1FF; }
static inline u64 pt_index(u64 va)   { return (va >> 12) & 0x1FF; }

// Clear a page
static void clear_page(u64 phys_addr) {
    u64 virt_addr = phys_to_virt(phys_addr);
    u8 *page = (u8*)virt_addr;
    for (int i = 0; i < 4096; i++) {
        page[i] = 0;
    }
}

// Full TLB flush via CR3 reload - essential after creating new page table hierarchy
static inline void tlb_flush_all(void) {
    u64 cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    __asm__ volatile("mov %0, %%cr3" : : "r"(cr3) : "memory");
    serial_puts("[USER_MAP] TLB flushed completely (CR3 reload)\r\n");
}

// Get or create a page table, ensuring U=1 at all levels
static u64* get_or_make_table(u64 *table, u64 index) {
    if (!(table[index] & PTE_PRESENT)) {
        // Allocate new page table
        u64 phys = pmm_alloc_page();
        if (!phys) {
            serial_puts("[USER_MAP] Failed to allocate page table\r\n");
            return NULL;
        }
        
        clear_page(phys);
        
        // Set entry with U=1 so user mode can traverse
        table[index] = phys | PTE_PRESENT | PTE_USER | PTE_WRITABLE;
        
        serial_puts("[USER_MAP] Created new page table at level\r\n");
    } else {
        // Ensure existing entry has U=1 for user traversal
        table[index] |= PTE_USER;
    }
    
    return (u64*)phys_to_virt(table[index] & PTE_ADDR_MASK);
}

// Map a 4K user page with U=1 at all levels
bool map_user_4k(u64 user_va, u64 phys_addr, u64 flags) {
    serial_puts("[USER_MAP] Mapping user VA to physical PA with flags\r\n");
    
    // Get current page table
    u64 cr3 = read_cr3();
    u64 *pml4 = (u64*)phys_to_virt(cr3 & PTE_ADDR_MASK);
    
    // Calculate indices
    u64 i4 = pml4_index(user_va);
    u64 i3 = pdpt_index(user_va);
    u64 i2 = pd_index(user_va);
    u64 i1 = pt_index(user_va);
    
    serial_puts("[USER_MAP] Page table walk: PML4->PDPT->PD->PT\r\n");
    
    // Walk/create page table hierarchy with U=1 at each level
    u64 *pdpt = get_or_make_table(pml4, i4);
    if (!pdpt) return false;
    
    // Ensure no 1GB pages (PS=0 in PDPTE)
    if (pdpt[i3] & PTE_HUGEPAGE) {
        serial_puts("[USER_MAP] ERROR: 1GB page found, cannot split\r\n");
        return false;
    }
    
    u64 *pd = get_or_make_table(pdpt, i3);
    if (!pd) return false;
    
    // Ensure no 2MB pages (PS=0 in PDE)  
    if (pd[i2] & PTE_HUGEPAGE) {
        serial_puts("[USER_MAP] ERROR: 2MB page found, cannot split\r\n");
        return false;
    }
    
    u64 *pt = get_or_make_table(pd, i2);
    if (!pt) return false;
    
    // Set the final PTE with user flags
    pt[i1] = (phys_addr & PTE_ADDR_MASK) | flags | PTE_USER;
    
    // CRITICAL: Full TLB flush after creating new page table hierarchy
    // invlpg() only flushes the final translation, not paging-structure caches
    tlb_flush_all();
    
    serial_puts("[USER_MAP] Successfully mapped user page\r\n");
    return true;
}

// Create user address space with code and stack pages
bool setup_user_address_space(void **user_code_va, void **user_stack_top, u64 code_size) {
    (void)code_size; // Suppress unused parameter warning
    serial_puts("[USER_MAP] Setting up user address space\r\n");
    
    // User virtual addresses (low canonical)
    u64 code_va = 0x0000000000400000;   // 4MB
    u64 stack_base = 0x0000000000800000; // 8MB
    u64 stack_size = 0x2000; // 8KB (2 pages)
    
    // Allocate physical pages
    u64 code_pa = pmm_alloc_page();
    u64 stack1_pa = pmm_alloc_page(); // Bottom page
    u64 stack0_pa = pmm_alloc_page(); // Top page
    
    if (!code_pa || !stack1_pa || !stack0_pa) {
        serial_puts("[USER_MAP] Failed to allocate physical pages\r\n");
        if (code_pa) pmm_free_page(code_pa);
        if (stack1_pa) pmm_free_page(stack1_pa);
        if (stack0_pa) pmm_free_page(stack0_pa);
        return false;
    }
    
    serial_puts("[USER_MAP] Allocated physical pages for code and stack\r\n");
    
    // Map code page: U=1, writable first (for copying), then will change to executable
    if (!map_user_4k(code_va, code_pa, USER_PAGE_WRITE)) {
        serial_puts("[USER_MAP] Failed to map code page as writable\r\n");
        return false;
    }
    
    // Map stack pages: U=1, writable, non-executable
    if (!map_user_4k(stack_base, stack1_pa, USER_PAGE_RW_NOEXEC)) {
        serial_puts("[USER_MAP] Failed to map stack page 1\r\n");
        return false;
    }
    
    if (!map_user_4k(stack_base + 0x1000, stack0_pa, USER_PAGE_RW_NOEXEC)) {
        serial_puts("[USER_MAP] Failed to map stack page 0\r\n");
        return false;
    }
    
    // Return user virtual addresses
    *user_code_va = (void*)code_va;
    *user_stack_top = (void*)(stack_base + stack_size); // Top of stack
    
    serial_puts("[USER_MAP] User address space setup complete\r\n");
    return true;
}

// Copy user code to user-accessible memory
bool copy_user_code(void *user_code_va, const void *kernel_code, u64 code_size) {
    serial_puts("[USER_MAP] Copying user code to user pages\r\n");
    
    if (code_size > 4096) {
        serial_puts("[USER_MAP] Code too large for single page\r\n");
        return false;
    }
    
    serial_puts("[USER_MAP] About to copy code - testing first byte\r\n");
    
    // Test SMAP - try to write one byte first
    u8 *dst = (u8*)user_code_va;
    const u8 *src = (const u8*)kernel_code;
    
    // If SMAP is enabled, this might fault. Let's see if we get here.
    serial_puts("[USER_MAP] Writing first byte...\r\n");
    dst[0] = src[0];
    serial_puts("[USER_MAP] First byte written successfully\r\n");
    
    // Copy the rest
    for (u64 i = 1; i < code_size; i++) {
        dst[i] = src[i];
    }
    
    serial_puts("[USER_MAP] User code copied successfully\r\n");
    return true;
}

// Change code page from writable to executable (W^X enforcement)
bool make_code_page_executable(void *user_code_va) {
    serial_puts("[USER_MAP] Making code page executable (removing write)\r\n");
    
    u64 code_va = (u64)user_code_va;
    
    // Get current page table
    u64 cr3 = read_cr3();
    u64 *pml4 = (u64*)phys_to_virt(cr3 & PTE_ADDR_MASK);
    
    // Walk page table to find the PTE
    u64 i4 = pml4_index(code_va);
    u64 i3 = pdpt_index(code_va);
    u64 i2 = pd_index(code_va);
    u64 i1 = pt_index(code_va);
    
    u64 *pdpt = (u64*)phys_to_virt(pml4[i4] & PTE_ADDR_MASK);
    u64 *pd = (u64*)phys_to_virt(pdpt[i3] & PTE_ADDR_MASK);
    u64 *pt = (u64*)phys_to_virt(pd[i2] & PTE_ADDR_MASK);
    
    // Clear writable bit, ensure executable (clear NX)
    pt[i1] &= ~PTE_WRITABLE;  // Remove write permission
    pt[i1] &= ~PTE_NOEXECUTE; // Remove NX bit (allow execution)
    
    // For leaf permission changes on existing mappings, invlpg is sufficient
    invlpg(code_va);
    
    serial_puts("[USER_MAP] Code page is now executable (W^X enforced)\r\n");
    return true;
}