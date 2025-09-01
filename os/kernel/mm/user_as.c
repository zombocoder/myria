#include <myria/types.h>
#include <myria/kapi.h>

// Page table flags
#define PTE_PRESENT     (1ULL << 0)
#define PTE_WRITABLE    (1ULL << 1)
#define PTE_USER        (1ULL << 2)
#define PTE_HUGEPAGE    (1ULL << 7)
#define PTE_GLOBAL      (1ULL << 8)
#define PTE_NOEXECUTE   (1ULL << 63)
#define PTE_ADDR_MASK   0x000FFFFFFFFFF000ULL

// Global kernel PML4 template for sharing kernel high-half
static u64 kernel_pml4_template_phys = 0;

// Helper functions
static inline u64 read_cr3(void) {
    u64 val;
    __asm__ volatile("mov %%cr3, %0" : "=r"(val));
    return val;
}

static inline void write_cr3(u64 val) {
    __asm__ volatile("mov %0, %%cr3" : : "r"(val) : "memory");
}

static u64 alloc_zeroed_page_phys(void) {
    u64 pa = pmm_alloc_page();
    if (!pa) return 0;
    
    // Zero the page via HHDM
    u8 *page = (u8*)phys_to_virt(pa);
    for (int i = 0; i < 4096; i++) {
        page[i] = 0;
    }
    return pa;
}

// Initialize kernel PML4 template - call once after kernel mappings are established
void init_kernel_pml4_template(void) {
    serial_puts("[USER_AS] Creating kernel PML4 template for high-half sharing\r\n");
    
    u64 cr3 = read_cr3();
    u64 *current_pml4 = (u64*)phys_to_virt(cr3 & PTE_ADDR_MASK);
    
    kernel_pml4_template_phys = alloc_zeroed_page_phys();
    if (!kernel_pml4_template_phys) {
        serial_puts("[USER_AS] ERROR: Failed to allocate kernel template PML4\r\n");
        return;
    }
    
    u64 *template = (u64*)phys_to_virt(kernel_pml4_template_phys);
    
    // Copy only high-half entries (256-511) - kernel mappings
    for (int i = 256; i < 512; i++) {
        template[i] = current_pml4[i];  // Share kernel mappings (U=0, GLOBAL OK)
    }
    
    serial_puts("[USER_AS] Kernel PML4 template initialized with high-half mappings\r\n");
}

// Create new user address space with isolated PML4
u64 user_as_create(void) {
    if (!kernel_pml4_template_phys) {
        serial_puts("[USER_AS] ERROR: Kernel template not initialized!\r\n");
        return 0;
    }
    
    serial_puts("[USER_AS] Creating new user address space\r\n");
    
    u64 user_pml4_phys = alloc_zeroed_page_phys();
    if (!user_pml4_phys) {
        serial_puts("[USER_AS] ERROR: Failed to allocate user PML4\r\n");
        return 0;
    }
    
    u64 *user_pml4 = (u64*)phys_to_virt(user_pml4_phys);
    u64 *template = (u64*)phys_to_virt(kernel_pml4_template_phys);
    
    // Copy kernel high-half mappings (entries 256-511)
    for (int i = 256; i < 512; i++) {
        user_pml4[i] = template[i];
    }
    
    // Low-half (0-255) remains zero - user space
    
    serial_puts("[USER_AS] User PML4 created with kernel high-half cloned\r\n");
    return user_pml4_phys;
}

// Get or create page table in specific PML4 with proper U=1 flags
static u64* get_or_make_table_in_pml4(u64 table_phys, u64 index, u64 upper_flags) {
    u64 *table = (u64*)phys_to_virt(table_phys);
    
    if (!(table[index] & PTE_PRESENT)) {
        u64 new_phys = alloc_zeroed_page_phys();
        if (!new_phys) return NULL;
        
        // Upper-level entries need P|W|U for user traversal
        table[index] = new_phys | PTE_PRESENT | PTE_WRITABLE | upper_flags;
        serial_puts("[USER_AS] Created new page table level\r\n");
    } else {
        // Ensure existing entry has required flags
        table[index] |= upper_flags;
    }
    
    return (u64*)phys_to_virt(table[index] & PTE_ADDR_MASK);
}

// Map 4K user page in specific PML4 (NOT the active one)
bool user_map_4k_in_pml4(u64 pml4_phys, u64 va, u64 pa, bool writable, bool executable) {
    serial_puts("[USER_AS] Mapping page in isolated PML4\r\n");
    
    // Calculate page table indices
    u64 i4 = (va >> 39) & 0x1FF;  // PML4 index
    u64 i3 = (va >> 30) & 0x1FF;  // PDPT index
    u64 i2 = (va >> 21) & 0x1FF;  // PD index
    u64 i1 = (va >> 12) & 0x1FF;  // PT index
    
    // Walk/create page table hierarchy with U=1 at each level for user traversal
    u64 *pdpt = get_or_make_table_in_pml4(pml4_phys, i4, PTE_USER);
    if (!pdpt) return false;
    
    u64 pdpt_phys = ((u64*)phys_to_virt(pml4_phys))[i4] & PTE_ADDR_MASK;
    u64 *pd = get_or_make_table_in_pml4(pdpt_phys, i3, PTE_USER);
    if (!pd) return false;
    
    u64 pd_phys = pdpt[i3] & PTE_ADDR_MASK;
    u64 *pt = get_or_make_table_in_pml4(pd_phys, i2, PTE_USER);
    if (!pt) return false;
    
    // Set final PTE with proper user flags (no GLOBAL for user pages)
    u64 flags = PTE_PRESENT | PTE_USER;
    if (writable) flags |= PTE_WRITABLE;
    if (!executable) flags |= PTE_NOEXECUTE;
    
    pt[i1] = (pa & PTE_ADDR_MASK) | flags;
    
    // Ensure page table writes are committed to memory
    __asm__ volatile("mfence" : : : "memory");
    __asm__ volatile("clflush (%0)" : : "r"(&pt[i1]) : "memory");
    __asm__ volatile("mfence" : : : "memory");
    
    // Debug: verify U=1 flags at all levels
    u64 *pml4_check = (u64*)phys_to_virt(pml4_phys);
    u64 *pd_check = (u64*)phys_to_virt(pdpt[i3] & PTE_ADDR_MASK);
    serial_puts("[USER_AS] DEBUG: Checking U=1 flags:\r\n");
    serial_puts("[USER_AS] PML4 entry has U=1: ");
    serial_puts((pml4_check[i4] & PTE_USER) ? "YES" : "NO");
    serial_puts("\r\n[USER_AS] PDPT entry has U=1: ");
    serial_puts((pdpt[i3] & PTE_USER) ? "YES" : "NO");
    serial_puts("\r\n[USER_AS] PD entry has U=1: ");
    serial_puts((pd_check[i2] & PTE_USER) ? "YES" : "NO");
    serial_puts("\r\n[USER_AS] PT entry has U=1: ");
    serial_puts((pt[i1] & PTE_USER) ? "YES" : "NO");
    serial_puts("\r\n");
    
    serial_puts("[USER_AS] Page mapped successfully in isolated PML4\r\n");
    return true;
}

// Manual page walk to verify what the CPU sees
static void debug_manual_page_walk(u64 pml4_phys, u64 va) {
    serial_puts("[USER_AS] MANUAL PAGE WALK for user code VA\r\n");
    
    u64 i4 = (va >> 39) & 0x1FF;
    u64 i3 = (va >> 30) & 0x1FF; 
    u64 i2 = (va >> 21) & 0x1FF;
    u64 i1 = (va >> 12) & 0x1FF;
    
    u64 *pml4 = (u64*)phys_to_virt(pml4_phys);
    serial_puts("[USER_AS] PML4[");
    // serial_puts hex number here would be nice but we don't have printf
    serial_puts("] = ");
    if (pml4[i4] & PTE_PRESENT) {
        serial_puts("PRESENT ");
        serial_puts((pml4[i4] & PTE_USER) ? "USER " : "KERNEL ");
    } else {
        serial_puts("NOT_PRESENT - WALK STOPS HERE!\r\n");
        return;
    }
    serial_puts("\r\n");
    
    u64 *pdpt = (u64*)phys_to_virt(pml4[i4] & PTE_ADDR_MASK);
    serial_puts("[USER_AS] PDPT[");
    serial_puts("] = ");
    if (pdpt[i3] & PTE_PRESENT) {
        serial_puts("PRESENT ");
        serial_puts((pdpt[i3] & PTE_USER) ? "USER " : "KERNEL ");
    } else {
        serial_puts("NOT_PRESENT - WALK STOPS HERE!\r\n");
        return;
    }
    serial_puts("\r\n");
    
    u64 *pd = (u64*)phys_to_virt(pdpt[i3] & PTE_ADDR_MASK);
    serial_puts("[USER_AS] PD[");
    serial_puts("] = ");
    if (pd[i2] & PTE_PRESENT) {
        serial_puts("PRESENT ");
        serial_puts((pd[i2] & PTE_USER) ? "USER " : "KERNEL ");
    } else {
        serial_puts("NOT_PRESENT - WALK STOPS HERE!\r\n");
        return;
    }
    serial_puts("\r\n");
    
    u64 *pt = (u64*)phys_to_virt(pd[i2] & PTE_ADDR_MASK);
    serial_puts("[USER_AS] PT[");
    serial_puts("] = ");
    if (pt[i1] & PTE_PRESENT) {
        serial_puts("PRESENT ");
        serial_puts((pt[i1] & PTE_USER) ? "USER " : "KERNEL ");
        serial_puts((pt[i1] & PTE_NOEXECUTE) ? "NX " : "EXEC ");
    } else {
        serial_puts("NOT_PRESENT - WALK STOPS HERE!\r\n");
        return;
    }
    serial_puts("\r\n[USER_AS] Manual walk successful - page should be accessible!\r\n");
}

// Create complete user address space with code and stack
u64 create_user_process(void) {
    serial_puts("[USER_AS] Creating complete user process address space\r\n");
    
    // Create separate user PML4
    u64 user_pml4 = user_as_create();
    if (!user_pml4) return 0;
    
    // Allocate physical frames for user pages
    u64 code_pa = alloc_zeroed_page_phys();
    u64 stack1_pa = alloc_zeroed_page_phys(); 
    u64 stack0_pa = alloc_zeroed_page_phys();
    
    if (!code_pa || !stack1_pa || !stack0_pa) {
        serial_puts("[USER_AS] ERROR: Failed to allocate user physical pages\r\n");
        return 0;
    }
    
    // Copy user payload to code page via HHDM (not user VA)
    extern u8 user_payload_start[];
    extern u8 user_payload_end[];
    u64 payload_size = (u64)(user_payload_end - user_payload_start);
    
    u8 *code_page = (u8*)phys_to_virt(code_pa);
    for (u64 i = 0; i < payload_size && i < 4096; i++) {
        code_page[i] = user_payload_start[i];
    }
    
    // Debug: verify code was copied correctly
    serial_puts("[USER_AS] User code copied. First 16 bytes: ");
    for (u64 i = 0; i < 16 && i < payload_size; i++) {
        u8 byte = code_page[i];
        // Simple hex display without printf
        if (byte != 0) {
            serial_puts("XX ");
        } else {
            serial_puts("00 ");
        }
    }
    serial_puts("\r\n[USER_AS] User code copy verified\r\n");
    
    // Flush caches after copying code to ensure coherency
    __asm__ volatile("mfence" : : : "memory");
    __asm__ volatile("clflush (%0)" : : "r"(code_page) : "memory");
    __asm__ volatile("mfence" : : : "memory");
    
    // Try a different virtual address to eliminate conflicts - use 0x10000 instead
    if (!user_map_4k_in_pml4(user_pml4, 0x0000000000010000ULL, code_pa, false, true)) {
        serial_puts("[USER_AS] ERROR: Failed to map code page\r\n");
        return 0;
    }
    
    if (!user_map_4k_in_pml4(user_pml4, 0x0000000000800000ULL, stack1_pa, true, false)) {
        serial_puts("[USER_AS] ERROR: Failed to map stack page 1\r\n");
        return 0;
    }
    
    if (!user_map_4k_in_pml4(user_pml4, 0x0000000000801000ULL, stack0_pa, true, false)) {
        serial_puts("[USER_AS] ERROR: Failed to map stack page 0\r\n");
        return 0;
    }
    
    serial_puts("[USER_AS] Complete user address space created\r\n");
    return user_pml4;
}

// Switch to user address space and enter user mode
void switch_to_user_process(u64 user_pml4_phys) {
    serial_puts("[USER_AS] Switching CR3 to user address space\r\n");
    
    // Clean CR3 switch - this gives us fresh paging-structure state
    write_cr3(user_pml4_phys);
    
    // Additional synchronization - switch CR3 multiple times to ensure it takes
    __asm__ volatile("mfence" : : : "memory");
    __asm__ volatile("mov %0, %%cr3" : : "r"(user_pml4_phys) : "memory");  
    __asm__ volatile("mfence" : : : "memory");
    __asm__ volatile("mov %0, %%cr3" : : "r"(user_pml4_phys) : "memory");
    __asm__ volatile("mfence" : : : "memory");
    
    // Specific page invalidations
    __asm__ volatile("invlpg (%0)" : : "r"(0x400000ULL) : "memory");
    __asm__ volatile("invlpg (%0)" : : "r"(0x800000ULL) : "memory"); 
    __asm__ volatile("invlpg (%0)" : : "r"(0x801000ULL) : "memory");
    
    // Pause to let CPU synchronize
    for (volatile int i = 0; i < 1000; i++) {
        __asm__ volatile("pause");
    }
    
    serial_puts("[USER_AS] CR3 switched with aggressive TLB flushing\r\n");
    
    // Debug: verify CR3 is actually set correctly
    u64 current_cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(current_cr3));
    serial_puts("[USER_AS] Expected CR3: ");
    // We can't print hex easily, but we can check if they match
    if ((current_cr3 & 0xFFFFFFFFFFFFF000ULL) == (user_pml4_phys & 0xFFFFFFFFFFFFF000ULL)) {
        serial_puts("MATCHES expected PML4!\r\n");
    } else {
        serial_puts("MISMATCH! CR3 not set to our user PML4!\r\n");
        // This would be a smoking gun
    }
    
    // Debug: manual page walk after CR3 switch  
    debug_manual_page_walk(user_pml4_phys, 0x10000ULL);
    
    // User entry points
    u64 user_rip = 0x0000000000010000ULL;  // Code page at new address
    u64 user_rsp = 0x0000000000802000ULL;  // Top of stack (stack0_pa + 4K)
    u64 user_rflags = 0x202;               // IF=1, reserved=1
    
    serial_puts("[USER_AS] Entering user mode with clean address space\r\n");
    enter_user(user_rip, user_rsp, user_rflags);
}