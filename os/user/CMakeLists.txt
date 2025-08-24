# Userland build configuration (disabled for M0)
# Will be enabled when implementing userland in later milestones

# set(CMAKE_C_STANDARD 17)
# set(CMAKE_C_STANDARD_REQUIRED ON)

# Userland compilation flags (static linking)
# set(USER_CFLAGS
#     -static
#     -nostdlib
#     -fno-builtin
#     -O2
#     -Wall
#     -Wextra
# )

# set(USER_LDFLAGS
#     -static
#     -nostdlib
# )

# Build C runtime - TODO: M2 milestone
# add_subdirectory(crt)

# Build minimal libc - TODO: M2 milestone  
# add_subdirectory(libc-mini)

# Build system services - TODO: M2 milestone
# add_subdirectory(init)