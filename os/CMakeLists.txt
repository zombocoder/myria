cmake_minimum_required(VERSION 3.20)

project(myria-os 
    VERSION 0.1.0
    LANGUAGES C ASM
    DESCRIPTION "Myria OS - x86-64 kernel with live migration"
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Cross-compilation setup for x86_64 kernel
if(NOT CMAKE_CROSSCOMPILING)
    message(FATAL_ERROR "This project requires cross-compilation. Set CMAKE_TOOLCHAIN_FILE.")
endif()

# Kernel compilation flags
set(KERNEL_CFLAGS
    -ffreestanding
    -fno-builtin
    -fno-stack-protector
    -mno-red-zone
    -mno-mmx
    -mno-sse
    -nostdlib
    -static
    -O2
    -Wall
    -Wextra
    -Werror
    -mcmodel=kernel
)

set(KERNEL_LDFLAGS
    -nostdlib
    -static
    -z max-page-size=0x1000
)

# Global include directories
include_directories(kernel/include)

# Build kernel
add_subdirectory(kernel)

# Build userland
add_subdirectory(user)

# Custom target for ISO creation
add_custom_target(iso
    COMMAND ${CMAKE_COMMAND} -E echo "Building ISO..."
    COMMAND make -C ${CMAKE_CURRENT_SOURCE_DIR} iso
    DEPENDS kernel
    COMMENT "Creating bootable ISO"
)

# Custom target for QEMU testing
add_custom_target(qemu
    COMMAND ${CMAKE_COMMAND} -E echo "Starting QEMU..."
    COMMAND make -C ${CMAKE_CURRENT_SOURCE_DIR} qemu
    DEPENDS iso
    COMMENT "Running in QEMU"
)

# Custom target for QEMU with debug
add_custom_target(qemu-debug
    COMMAND ${CMAKE_COMMAND} -E echo "Starting QEMU with debug..."
    COMMAND make -C ${CMAKE_CURRENT_SOURCE_DIR} qemu-debug
    DEPENDS iso
    COMMENT "Running in QEMU with debug flags"
)