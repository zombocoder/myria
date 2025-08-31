#ifndef MYRIA_SYSCALLS_H
#define MYRIA_SYSCALLS_H

#include <myria/types.h>

// System call numbers
#define SYS_EXIT        0
#define SYS_WRITE       1
#define SYS_READ        2
#define SYS_OPEN        3
#define SYS_CLOSE       4
#define SYS_FORK        5
#define SYS_EXECVE      6
#define SYS_GETPID      7
#define SYS_SLEEP       8
#define SYS_YIELD       9
#define SYS_MALLOC      10
#define SYS_FREE        11

// System call wrapper functions for user programs
static inline void sys_exit(u64 exit_code) {
    syscall_dispatch(SYS_EXIT, exit_code, 0, 0, 0, 0, 0);
}

static inline u64 sys_write(u64 fd, const void *buf, u64 count) {
    return syscall_dispatch(SYS_WRITE, fd, (u64)buf, count, 0, 0, 0);
}

static inline u64 sys_read(u64 fd, void *buf, u64 count) {
    return syscall_dispatch(SYS_READ, fd, (u64)buf, count, 0, 0, 0);
}

static inline u64 sys_getpid(void) {
    return syscall_dispatch(SYS_GETPID, 0, 0, 0, 0, 0, 0);
}

static inline void sys_sleep(u64 milliseconds) {
    syscall_dispatch(SYS_SLEEP, milliseconds, 0, 0, 0, 0, 0);
}

static inline void sys_yield(void) {
    syscall_dispatch(SYS_YIELD, 0, 0, 0, 0, 0, 0);
}

static inline void* sys_malloc(u64 size) {
    return (void*)syscall_dispatch(SYS_MALLOC, size, 0, 0, 0, 0, 0);
}

static inline void sys_free(void *ptr) {
    syscall_dispatch(SYS_FREE, (u64)ptr, 0, 0, 0, 0, 0);
}

// System call dispatcher (implemented in syscalls.c)
extern u64 syscall_dispatch(u64 syscall_num, u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5, u64 arg6);

#endif // MYRIA_SYSCALLS_H