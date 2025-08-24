#include <myria/types.h>
#include <myria/kapi.h>

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

// CR0 flags
#define CR0_PG          (1UL << 31)
#define CR0_WP          (1UL << 16)

// CR4 flags  
#define CR4_PAE         (1UL << 5)
#define CR4_PGE         (1UL << 7)

// EFER flags
#define EFER_LME        (1UL << 8)
#define EFER_LMA        (1UL << 10)

// MSR numbers
#define MSR_EFER        0xC0000080

// No static page tables - rely on Limine's initial setup

void setup_initial_paging(void) {
    // For now, rely on Limine's initial paging setup
    // This function will be expanded later when we have dynamic allocation
    kprintf("[PAGING] Using Limine's initial page tables\n");
}

void enable_paging(void) {
    // Paging is already enabled by Limine - nothing to do
    kprintf("[PAGING] Paging already enabled by bootloader\n");
}