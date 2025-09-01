# User mode payload - ULTRA SIMPLE test without any addressing
.section .text
.intel_syntax noprefix

# This code will be copied to user virtual address 0x10000
# and executed in ring 3

.global user_payload_start
.global user_payload_end

user_payload_start:
    # Simplest possible test - just exit immediately
    mov rax, 0          # SYS_EXIT = 0
    mov rdi, 99         # exit code = 99 (distinctive value)
    syscall
    
    # Should never reach here
    hlt
    hlt
    hlt

user_payload_end:

.att_syntax prefix