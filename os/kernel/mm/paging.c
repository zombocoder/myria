#include <myria/types.h>
#include <myria/kapi.h>

// External HHDM request with response pointer  
extern struct limine_hhdm_response *limine_hhdm_request[6];

// Page table entry flags
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

// Forward declarations
extern u64 __kernel_start, __kernel_end;
extern u64 __text_start, __text_end;
extern u64 __rodata_start, __rodata_end;
extern u64 __data_start, __data_end;
extern u64 __bss_start, __bss_end;

// Helper functions
static inline u64 read_cr3(void) {
    u64 val;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(val));
    return val;
}

static inline void write_cr3(u64 val) {
    __asm__ volatile ("mov %0, %%cr3" :: "r"(val) : "memory");
}

static inline void invlpg(u64 addr) {
    __asm__ volatile ("invlpg (%0)" :: "r"(addr) : "memory");
}

// Get HHDM base from Limine
static u64 get_hhdm_base(void) {
    // Access the response pointer (6th element of the request structure)
    struct limine_hhdm_response *hhdm_response = limine_hhdm_request[5];
    
    if (hhdm_response) {
        serial_puts("[PAGING] HHDM response found\r\n");
        return hhdm_response->offset;
    }
    
    serial_puts("[PAGING] No HHDM response - using fallback\r\n");
    return 0xffff800000000000ULL;  // Common HHDM base fallback
}

// Convert physical address to virtual using HHDM (no static variables!)
static inline u64 phys_to_virt(u64 phys_addr) {
    // Get HHDM base each time - no static variables during bootstrap!
    u64 hhdm_base = get_hhdm_base();
    return phys_addr + hhdm_base;
}

// Unused functions removed to fix compiler warnings

// Debug function to inspect current page table entry
void debug_page_mapping(u64 virt_addr) {
    u64 cr3 = read_cr3();
    u64 *pml4 = (u64*)((cr3 & PTE_ADDR_MASK) + 0xffffffff80000000);
    
    u64 pml4_idx = (virt_addr >> 39) & 0x1FF;
    u64 pdpt_idx = (virt_addr >> 30) & 0x1FF;
    u64 pd_idx = (virt_addr >> 21) & 0x1FF;
    u64 pt_idx = (virt_addr >> 12) & 0x1FF;
    
    serial_puts("[DEBUG] Page mapping for address:\r\n");
    
    if (!(pml4[pml4_idx] & PTE_PRESENT)) {
        serial_puts("[DEBUG] PML4 entry not present\r\n");
        return;
    }
    
    u64 *pdpt = (u64*)(((pml4[pml4_idx] & PTE_ADDR_MASK) + 0xffffffff80000000));
    if (!(pdpt[pdpt_idx] & PTE_PRESENT)) {
        serial_puts("[DEBUG] PDPT entry not present\r\n");
        return;
    }
    
    u64 *pd = (u64*)(((pdpt[pdpt_idx] & PTE_ADDR_MASK) + 0xffffffff80000000));
    if (!(pd[pd_idx] & PTE_PRESENT)) {
        serial_puts("[DEBUG] PD entry not present\r\n");
        return;
    }
    
    u64 *pt = (u64*)(((pd[pd_idx] & PTE_ADDR_MASK) + 0xffffffff80000000));
    u64 pte = pt[pt_idx];
    
    if (!(pte & PTE_PRESENT)) {
        serial_puts("[DEBUG] PTE not present\r\n");
        return;
    }
    
    if (pte & PTE_WRITABLE) {
        serial_puts("[DEBUG] Page is WRITABLE\r\n");
    } else {
        serial_puts("[DEBUG] Page is READ-ONLY\r\n");
    }
}

// Targeted page table walk to find and modify BSS pages only
typedef enum { LEAF_4K, LEAF_2M, LEAF_1G, LEAF_NONE } leaf_t;

typedef struct {
    u64 *pml4;
    u64 *pml4e;
    u64 *pdpt;
    u64 *pdpte;
    u64 *pd;
    u64 *pde;
    u64 *pt;
    u64 *pte;
    leaf_t leaf;
} walk_t;

static inline u64 pml4_index(u64 va) { return (va >> 39) & 0x1FF; }
static inline u64 pdpt_index(u64 va) { return (va >> 30) & 0x1FF; }
static inline u64 pd_index(u64 va)   { return (va >> 21) & 0x1FF; }
static inline u64 pt_index(u64 va)   { return (va >> 12) & 0x1FF; }

static bool walk_va_to_leaf(u64 va, walk_t *w) {
    u64 cr3 = read_cr3();
    w->pml4 = (u64*)phys_to_virt(cr3 & PTE_ADDR_MASK);
    
    w->pml4e = &w->pml4[pml4_index(va)];  // Use ACTUAL VA, not hard-coded!
    if (!(*w->pml4e & PTE_PRESENT)) return false;
    
    w->pdpt = (u64*)phys_to_virt((*w->pml4e) & PTE_ADDR_MASK);
    w->pdpte = &w->pdpt[pdpt_index(va)];
    if (!(*w->pdpte & PTE_PRESENT)) return false;
    
    if (*w->pdpte & PTE_HUGEPAGE) {  // 1GB leaf
        w->leaf = LEAF_1G;
        return true;
    }
    
    w->pd = (u64*)phys_to_virt((*w->pdpte) & PTE_ADDR_MASK);
    w->pde = &w->pd[pd_index(va)];
    if (!(*w->pde & PTE_PRESENT)) return false;
    
    if (*w->pde & PTE_HUGEPAGE) {  // 2MB leaf  
        w->leaf = LEAF_2M;
        return true;
    }
    
    w->pt = (u64*)phys_to_virt((*w->pde) & PTE_ADDR_MASK);
    w->pte = &w->pt[pt_index(va)];
    if (!(*w->pte & PTE_PRESENT)) return false;
    
    w->leaf = LEAF_4K;
    return true;
}

static bool make_page_writable_minimal(u64 va) {
    walk_t w;
    if (!walk_va_to_leaf(va, &w)) {
        serial_puts("[PAGING] Walk failed for address\r\n");
        return false;
    }
    
    switch (w.leaf) {
        case LEAF_4K:
            if (*w.pte & PTE_WRITABLE) {
                // Already writable
                return true;
            }
            serial_puts("[PAGING] Setting 4K page writable\r\n");
            *w.pte |= PTE_WRITABLE;
            invlpg(va);
            return true;
        case LEAF_2M:
            if (*w.pde & PTE_WRITABLE) {
                // Already writable
                return true;
            }
            serial_puts("[PAGING] Setting 2M huge page writable\r\n");
            *w.pde |= PTE_WRITABLE;
            invlpg(va & ~((1ULL << 21) - 1));  // Flush 2MB boundary
            return true;
        case LEAF_1G:
            if (*w.pdpte & PTE_WRITABLE) {
                // Already writable
                return true;
            }
            serial_puts("[PAGING] Setting 1G huge page writable\r\n");
            *w.pdpte |= PTE_WRITABLE;
            invlpg(va & ~((1ULL << 30) - 1));  // Flush 1GB boundary
            return true;
        default:
            serial_puts("[PAGING] Unknown leaf type\r\n");
            return false;
    }
}

// Complete approach - enable write permissions for ALL kernel writable sections
void enable_bss_write_permissions(void) {
    serial_puts("[PAGING] Enabling write permissions for ALL kernel writable sections\r\n");
    
    // Fix: Use the complete .data + .bss range as you suggested
    u64 start = (u64)&__data_start;
    u64 end = (u64)&__bss_end;
    
    serial_puts("[PAGING] Debug: Addresses to fix:\r\n");
    // We can't print addresses easily, but let's show range info
    
    u64 total_pages = ((end + 0xFFFULL) & ~0xFFFULL) - (start & ~0xFFFULL);
    total_pages /= 0x1000;
    
    serial_puts("[PAGING] Walking ALL kernel writable pages\r\n");
    
    for (u64 va = start & ~0xFFFULL; va < ((end + 0xFFFULL) & ~0xFFFULL); va += 0x1000) {
        if (!make_page_writable_minimal(va)) {
            serial_puts("[PAGING] Failed to make kernel writable page accessible\r\n");
            for(;;) { __asm__ volatile("cli; hlt"); }
        }
    }
    
    serial_puts("[PAGING] All kernel writable sections enabled successfully\r\n");
}

// Targeted approach - only enable write permissions for BSS
u64 setup_kernel_page_tables(void) {
    serial_puts("[PAGING] Using targeted BSS permission fix\r\n");
    
    enable_bss_write_permissions();
    
    // Return current CR3 - no change needed
    return read_cr3();
}

// Activate kernel page tables and clear BSS
void activate_kernel_page_tables(u64 pml4_phys) {
    serial_puts("[PAGING] Page tables already modified - no CR3 change needed\r\n");
    
    // Flush TLB to ensure new permissions take effect
    serial_puts("[PAGING] Flushing TLB\r\n");
    write_cr3(pml4_phys);  // Writing current CR3 flushes TLB
    
    serial_puts("[PAGING] TLB flushed\r\n");
    
    // NOW we can safely clear BSS since we have write permissions
    serial_puts("[PAGING] Clearing BSS section\r\n");
    u64 bss_size = (u64)&__bss_end - (u64)&__bss_start;
    u8 *bss_ptr = (u8*)&__bss_start;
    
    serial_puts("[PAGING] BSS range and size calculated\r\n");
    serial_puts("[PAGING] About to clear first BSS byte\r\n");
    
    // Test clearing just one byte first
    bss_ptr[0] = 0;
    
    serial_puts("[PAGING] First BSS byte cleared successfully!\r\n");
    serial_puts("[PAGING] BSS clearing progress: Start full clear\r\n");
    
    // Clear BSS in chunks to show progress
    for (u64 i = 0; i < bss_size; i += 4096) {  // Clear in 4KB chunks
        // Clear up to 4KB or remainder
        u64 chunk_size = (bss_size - i < 4096) ? (bss_size - i) : 4096;
        for (u64 j = 0; j < chunk_size; j++) {
            bss_ptr[i + j] = 0;
        }
        
        // Print progress every 64KB
        if (i % (64 * 1024) == 0) {
            serial_puts("[PAGING] BSS clearing progress: 64KB\r\n");
        }
    }
    
    serial_puts("[PAGING] BSS cleared successfully\r\n");
}