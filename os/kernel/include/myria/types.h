#ifndef MYRIA_TYPES_H
#define MYRIA_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;

typedef uintptr_t uptr;
typedef intptr_t  iptr;

typedef u64 paddr_t;  // Physical address
typedef u64 vaddr_t;  // Virtual address

// Page size constants
#define PAGE_SIZE       0x1000UL
#define PAGE_SHIFT      12
#define LARGE_PAGE_SIZE 0x200000UL  // 2MB
#define LARGE_PAGE_SHIFT 21

// Alignment macros
#define ALIGN_DOWN(x, align) ((x) & ~((align) - 1))
#define ALIGN_UP(x, align)   (((x) + (align) - 1) & ~((align) - 1))
#define PAGE_ALIGN(x)        ALIGN_UP(x, PAGE_SIZE)

// Bit manipulation
#define BIT(n)              (1UL << (n))
#define BITMASK(n)          (BIT(n) - 1)
#define SET_BIT(x, n)       ((x) |= BIT(n))
#define CLEAR_BIT(x, n)     ((x) &= ~BIT(n))
#define TEST_BIT(x, n)      (((x) & BIT(n)) != 0)

// Compiler attributes
#define PACKED          __attribute__((packed))
#define ALIGNED(n)      __attribute__((aligned(n)))
#define NORETURN        __attribute__((noreturn))
#define UNUSED          __attribute__((unused))
#define SECTION(s)      __attribute__((section(s)))

// Kernel memory layout (higher-half)
#define KERNEL_VMA_BASE     0xffffffff80000000UL
#define KERNEL_HEAP_BASE    0xffffffffa0000000UL
#define KERNEL_HEAP_SIZE    0x20000000UL  // 512MB heap
#define KERNEL_VMAP_BASE    0xffffffffc0000000UL
#define KERNEL_MMIO_BASE    0xffffffffe0000000UL
#define KERNEL_PERCPU_BASE  0xffffffffff000000UL

// Convert physical to virtual (direct mapping)
#define PHYS_TO_VIRT(paddr) ((void*)((paddr) + KERNEL_VMA_BASE))
#define VIRT_TO_PHYS(vaddr) ((paddr_t)((uptr)(vaddr) - KERNEL_VMA_BASE))

// Limine bootloader structures
#define LIMINE_MEMMAP_USABLE                 0
#define LIMINE_MEMMAP_RESERVED               1
#define LIMINE_MEMMAP_ACPI_RECLAIMABLE       2
#define LIMINE_MEMMAP_ACPI_NVS               3
#define LIMINE_MEMMAP_BAD_MEMORY             4
#define LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE 5
#define LIMINE_MEMMAP_KERNEL_AND_MODULES     6
#define LIMINE_MEMMAP_FRAMEBUFFER            7

struct limine_uuid {
    uint32_t a;
    uint16_t b;
    uint16_t c;
    uint8_t d[8];
};

struct limine_file {
    uint64_t revision;
    void *address;
    uint64_t size;
    char *path;
    char *cmdline;
    uint32_t media_type;
    uint32_t unused;
};

struct limine_memmap_entry {
    uint64_t base;
    uint64_t length;
    uint64_t type;
};

struct limine_memmap_response {
    uint64_t revision;
    uint64_t entry_count;
    struct limine_memmap_entry **entries;
};

struct limine_memmap_request {
    uint64_t id[4];
    uint64_t revision;
    struct limine_memmap_response *response;
};

struct limine_hhdm_response {
    uint64_t revision;
    uint64_t offset;
};

struct limine_hhdm_request {
    uint64_t id[4];
    uint64_t revision;
    struct limine_hhdm_response *response;
};

#endif // MYRIA_TYPES_H