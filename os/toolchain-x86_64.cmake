# CMake toolchain file for x86_64 cross-compilation

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Cross-compiler setup
set(CMAKE_C_COMPILER x86_64-elf-gcc)
set(CMAKE_ASM_COMPILER x86_64-elf-gcc)
set(CMAKE_LINKER x86_64-elf-ld)
set(CMAKE_AR x86_64-elf-ar)
set(CMAKE_RANLIB x86_64-elf-ranlib)
set(CMAKE_STRIP x86_64-elf-strip)

# Alternative: use clang if preferred
# set(CMAKE_C_COMPILER clang)
# set(CMAKE_ASM_COMPILER clang)
# set(CMAKE_C_COMPILER_TARGET x86_64-elf)
# set(CMAKE_ASM_COMPILER_TARGET x86_64-elf)

# Don't run the linker on compiler check
set(CMAKE_C_COMPILER_WORKS TRUE)
set(CMAKE_ASM_COMPILER_WORKS TRUE)

# Search paths
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Disable position independent code
set(CMAKE_POSITION_INDEPENDENT_CODE OFF)