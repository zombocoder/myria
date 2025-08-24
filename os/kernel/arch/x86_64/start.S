.section .text
.global _start
.code64

# Limine protocol structures
.section .requests
.global limine_base_revision
limine_base_revision:
    .quad 0xf9562b2d5c95a6c8
    .quad 0x6a7b384944536bdc

.global limine_memmap_request  
limine_memmap_request:
    .quad 0xc7b1dd30df4c8b88  # LIMINE_COMMON_MAGIC[0]
    .quad 0x0a82e883a194f07b  # LIMINE_COMMON_MAGIC[1]
    .quad 0x67cf3d9d378a806f  # MEMMAP_REQUEST_MAGIC[0]
    .quad 0xe304acdfc50c3c62  # MEMMAP_REQUEST_MAGIC[1]
    .quad 0  # revision
    .quad 0  # response pointer

.global limine_hhdm_request
limine_hhdm_request:
    .quad 0xc7b1dd30df4c8b88  # LIMINE_COMMON_MAGIC[0]
    .quad 0x0a82e883a194f07b  # LIMINE_COMMON_MAGIC[1]
    .quad 0x48dcf1cb8ad2b852  # HHDM_REQUEST_MAGIC[0]
    .quad 0x63984e959a98244b  # HHDM_REQUEST_MAGIC[1]
    .quad 0  # revision
    .quad 0  # response pointer

.section .text

# Entry point from Limine
_start:
    # Disable interrupts immediately
    cli
    
    # Use Limine's provided stack for now (don't set our own stack yet)
    # lea __stack_top(%rip), %rsp
    
    # Clear direction flag (for string operations)  
    cld
    
    # Skip BSS clearing - Limine doesn't map BSS section
    # lea __bss_start(%rip), %rdi
    # lea __bss_end(%rip), %rcx
    # sub %rdi, %rcx
    # xor %rax, %rax
    # rep stosb
    
    # Save Limine bootloader info (if needed later)
    # %rdi contains bootloader info pointer
    
    # Call early x86-64 initialization
    call x86_early_init
    
    # Jump to kernel main
    call kmain
    
    # Should never return, but just in case
.hang:
    cli
    hlt
    jmp .hang

# Exception handlers (basic stubs for now)
.global divide_error_handler
divide_error_handler:
    cli
    hlt
    jmp divide_error_handler

.global debug_handler  
debug_handler:
    cli
    hlt
    jmp debug_handler

.global nmi_handler
nmi_handler:
    cli
    hlt
    jmp nmi_handler

.global breakpoint_handler
breakpoint_handler:
    cli
    hlt
    jmp breakpoint_handler

.global overflow_handler
overflow_handler:
    cli
    hlt
    jmp overflow_handler

.global bound_range_handler
bound_range_handler:
    cli
    hlt
    jmp bound_range_handler

.global invalid_opcode_handler
invalid_opcode_handler:
    cli
    hlt
    jmp invalid_opcode_handler

.global device_not_available_handler
device_not_available_handler:
    cli
    hlt
    jmp device_not_available_handler

.global double_fault_handler
double_fault_handler:
    cli
    hlt
    jmp double_fault_handler

.global invalid_tss_handler
invalid_tss_handler:
    cli
    hlt
    jmp invalid_tss_handler

.global segment_not_present_handler
segment_not_present_handler:
    cli
    hlt
    jmp segment_not_present_handler

.global stack_segment_fault_handler
stack_segment_fault_handler:
    cli
    hlt
    jmp stack_segment_fault_handler

.global general_protection_handler
general_protection_handler:
    cli
    hlt
    jmp general_protection_handler

.global page_fault_handler
page_fault_handler:
    cli
    hlt
    jmp page_fault_handler

.global x87_fpu_error_handler
x87_fpu_error_handler:
    cli
    hlt
    jmp x87_fpu_error_handler

.global alignment_check_handler
alignment_check_handler:
    cli
    hlt
    jmp alignment_check_handler

.global machine_check_handler
machine_check_handler:
    cli
    hlt
    jmp machine_check_handler

.global simd_fpu_handler
simd_fpu_handler:
    cli
    hlt
    jmp simd_fpu_handler

.global virtualization_handler
virtualization_handler:
    cli
    hlt
    jmp virtualization_handler

# Kernel stack (32KB should be enough for early boot)
.section .bss
.align 16
kernel_stack:
    .skip 0x8000
kernel_stack_top: