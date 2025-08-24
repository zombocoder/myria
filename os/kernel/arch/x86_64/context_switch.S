.section .text

# void context_switch(cpu_context_t *old_ctx, cpu_context_t *new_ctx)
# RDI = old_ctx, RSI = new_ctx
.global context_switch
context_switch:
    # Save old context
    # Save general purpose registers
    movq %rax, 0(%rdi)      # old_ctx->rax
    movq %rbx, 8(%rdi)      # old_ctx->rbx
    movq %rcx, 16(%rdi)     # old_ctx->rcx
    movq %rdx, 24(%rdi)     # old_ctx->rdx
    movq %rsi, 32(%rdi)     # old_ctx->rsi (save new_ctx pointer)
    # Note: RDI (old_ctx) will be saved after we're done using it
    movq %rbp, 48(%rdi)     # old_ctx->rbp
    movq %rsp, 56(%rdi)     # old_ctx->rsp
    movq %r8,  64(%rdi)     # old_ctx->r8
    movq %r9,  72(%rdi)     # old_ctx->r9
    movq %r10, 80(%rdi)     # old_ctx->r10
    movq %r11, 88(%rdi)     # old_ctx->r11
    movq %r12, 96(%rdi)     # old_ctx->r12
    movq %r13, 104(%rdi)    # old_ctx->r13
    movq %r14, 112(%rdi)    # old_ctx->r14
    movq %r15, 120(%rdi)    # old_ctx->r15
    
    # Save RDI now that we're done using it
    movq %rdi, 40(%rdi)     # old_ctx->rdi
    
    # Save control registers
    # RIP is the return address on stack
    movq (%rsp), %rax
    movq %rax, 128(%rdi)    # old_ctx->rip
    
    # Save RFLAGS
    pushfq
    popq %rax
    movq %rax, 136(%rdi)    # old_ctx->rflags
    
    # Save segment registers (CS and SS)
    movw %cs, %ax
    movw %ax, 144(%rdi)     # old_ctx->cs
    movw %ss, %ax
    movw %ax, 146(%rdi)     # old_ctx->ss
    
    # Load new context (RSI = new_ctx)
    # Restore general purpose registers
    movq 0(%rsi), %rax      # new_ctx->rax
    movq 8(%rsi), %rbx      # new_ctx->rbx
    movq 16(%rsi), %rcx     # new_ctx->rcx
    movq 24(%rsi), %rdx     # new_ctx->rdx
    # Skip RSI and RDI for now
    movq 48(%rsi), %rbp     # new_ctx->rbp
    movq 64(%rsi), %r8      # new_ctx->r8
    movq 72(%rsi), %r9      # new_ctx->r9
    movq 80(%rsi), %r10     # new_ctx->r10
    movq 88(%rsi), %r11     # new_ctx->r11
    movq 96(%rsi), %r12     # new_ctx->r12
    movq 104(%rsi), %r13    # new_ctx->r13
    movq 112(%rsi), %r14    # new_ctx->r14
    movq 120(%rsi), %r15    # new_ctx->r15
    
    # Set up new stack
    movq 56(%rsi), %rsp     # new_ctx->rsp
    
    # Push new RIP onto new stack (for ret instruction)
    movq 128(%rsi), %r11    # new_ctx->rip
    pushq %r11
    
    # Push new RFLAGS onto stack
    movq 136(%rsi), %r11    # new_ctx->rflags
    pushq %r11
    
    # Restore RSI and RDI last
    movq 40(%rsi), %rdi     # new_ctx->rdi
    movq 32(%rsi), %r11     # new_ctx->rsi (into temp register)
    
    # Restore RFLAGS
    popfq
    
    # Move RSI from temp register
    movq %r11, %rsi
    
    # Jump to new thread's RIP
    ret

# Thread entry trampoline for new threads
.global thread_entry_trampoline
thread_entry_trampoline:
    # RDI contains entry_point, RSI contains arg
    # Call thread_wrapper(entry_point, arg)
    call thread_wrapper
    # Should never return
    cli
    hlt