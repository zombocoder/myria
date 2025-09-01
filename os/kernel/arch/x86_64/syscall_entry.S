# Fast syscall entry/exit for x86_64
.section .text
.intel_syntax noprefix

# Syscall entry point from user mode
.global syscall_entry
.type syscall_entry, @function
syscall_entry:
    # SYSCALL instruction has already:
    # - Saved user RIP to RCX
    # - Saved user RFLAGS to R11
    # - Loaded kernel CS/SS from STAR MSR
    # - Cleared RFLAGS according to SFMASK MSR
    # - Jumped to this address (LSTAR MSR)
    
    # Swapgs to get per-CPU data (not implemented yet, skip for now)
    # swapgs
    
    # Save user RSP in a temporary location (use simplified approach)
    # Store user RSP in a global variable for now (not multi-CPU safe)
    mov [saved_user_rsp], rsp
    
    # Load kernel syscall stack
    mov rsp, kernel_syscall_stack_top
    
    # Save user registers on kernel stack
    push r11        # User RFLAGS
    push rcx        # User RIP
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15
    
    # Convert from syscall registers to C calling convention:
    # RAX = syscall number -> RDI (1st arg)
    # RDI = arg1 -> RSI (2nd arg)  
    # RSI = arg2 -> RDX (3rd arg)
    # RDX = arg3 -> RCX (4th arg) 
    # R10 = arg4 -> R8 (5th arg)
    # R8 = arg5 -> R9 (6th arg)
    # R9 = arg6 -> stack (7th arg)
    
    # Save registers we'll overwrite
    push r9    # arg6 - will go on stack
    
    # Convert register arguments (work backwards to avoid conflicts)
    mov r8, r10   # arg4: R10 -> R8
    mov rcx, rdx  # arg3: RDX -> RCX  
    mov rdx, rsi  # arg2: RSI -> RDX
    mov rsi, rdi  # arg1: RDI -> RSI
    mov rdi, rax  # syscall_num: RAX -> RDI
    
    # Call the C syscall dispatcher
    call syscall_dispatch
    
    # Clean up stack (remove arg6)
    add rsp, 8
    
    # Result is in RAX - keep it there for return to user
    
    # Restore user registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    pop rcx         # User RIP
    pop r11         # User RFLAGS
    
    # Restore user RSP
    mov rsp, [saved_user_rsp]  # Get saved user RSP (simplified)
    
    # Swapgs to restore user GS (not implemented yet)
    # swapgs
    
    # Return to user mode
    # SYSRET will:
    # - Load RCX into RIP (user return address)
    # - Load R11 into RFLAGS (user flags)
    # - Load user CS/SS from STAR MSR
    sysretq

# Simplified kernel syscall stack (8KB)
.section .bss
.align 16
kernel_syscall_stack_bottom:
.skip 8192
kernel_syscall_stack_top:

# Temporary storage for user RSP during syscalls
saved_user_rsp:
.quad 0

.att_syntax prefix