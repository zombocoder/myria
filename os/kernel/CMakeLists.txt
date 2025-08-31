set(KERNEL_SOURCES
    minimal_test.c
    arch/x86_64/start.S
    arch/x86_64/portio.c
    util/serial.c
    mm/pmm_simple.c
    mm/paging.c
    mm/vmm.c
    sched/thread_minimal.c
    syscall/syscalls.c
)

# Apply kernel-specific flags
add_compile_options(${KERNEL_CFLAGS})
add_link_options(${KERNEL_LDFLAGS})

# Create kernel executable
add_executable(kernel.elf ${KERNEL_SOURCES})

# Set custom linker script
set_target_properties(kernel.elf PROPERTIES
    LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/arch/x86_64/linker.ld
)

target_link_options(kernel.elf PRIVATE
    -T ${CMAKE_CURRENT_SOURCE_DIR}/arch/x86_64/linker.ld
)

# Include directories specific to kernel
target_include_directories(kernel.elf PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/x86_64/include
)