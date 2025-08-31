.section .text
.global _start
.code64

# External function declarations
.extern x86_early_init
.extern kmain
.extern simple_test_function

# Debug macro for serial output - avoid stack operations
.macro DBG char
    mov $0x3f8, %dx
    mov $\char, %al
    out %al, %dx
.endm

# Limine protocol structures - correct format  
.section .limine_reqs,"aw"
.align 8

.global limine_base_revision
limine_base_revision:
    .quad 0xf9562b2d5c95a6c8
    .quad 0x6a7b384944536bdc  
    .quad 3  # Base revision 3

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
    
    # Clear direction flag (for string operations)  
    cld
    
    DBG 'A'  # Debug: reached start
    
    # Keep using Limine's stack for now - don't change RSP
    # Just ensure proper alignment for System V ABI if needed
    and $0xFFFFFFFFFFFFFFF0, %rsp   # Align to 16-byte boundary
    sub $8, %rsp                     # Make RSP % 16 == 8 before call (SysV ABI)
    
    DBG 'B'  # Debug: using bootloader stack
    
    # Skip BSS clearing - use explicit C initialization instead
    DBG 'X'  # Debug: skipping BSS clearing (problematic with current memory setup)
    # call clear_bss_safe  # Disabled - causes hang with higher-half addresses
    DBG 'C'  # Debug: proceeding without BSS clear
    
    # Set up basic virtual memory before calling C functions
    DBG '0'  # Debug: About to setup virtual memory (inline)
    
    # Inline the virtual memory setup - avoid function calls for now
    mov %cr3, %rax           # Read CR3 to verify paging is active
    DBG 'P'                  # Debug: Read CR3 successfully
    
    DBG 'V'  # Debug: Virtual memory setup complete
    
    # Debug: Stack should already be aligned from initial setup
    DBG '1'  # Debug: About to test simple function call
    
    # Test if we can even execute a simple instruction after debug
    DBG '2'  # Debug: Simple instruction works
    
    # First try calling the simplest possible C function
    DBG '3'  # Debug: About to call simple_test_function
    
    # Ensure interrupts are disabled (they should be already)
    cli
    DBG 'i'  # Debug: Interrupts disabled
    
    DBG 'j'  # Debug: After CLI
    
    # Just try outputting more debug characters to see if we can continue
    DBG 'W'  # Debug: Test point W
    
    # Success - call kernel functions
    call serial_init
    
    call x86_early_init  
    
    call kmain
    
    # Should never return, but just in case
.hang:
    cli
    hlt
    jmp .hang

# Set up basic page tables for kernel execution
setup_basic_paging:
    DBG 'P'  # Debug: entered setup_basic_paging
    
    # Simple test - just try to read CR3 and return
    mov %cr3, %rax
    DBG 'T'  # Debug: got CR3
    
    # For now, just return - we'll assume bootloader paging is sufficient
    DBG 'Q'  # Debug: using bootloader paging
    
    ret

# Clear BSS section - Conservative Linux-style 32-bit implementation  
clear_bss_safe:
    DBG 'b'  # Debug: entered clear_bss_safe
    
    # Load BSS start and end addresses
    lea __bss_start(%rip), %rdi      # Destination: start of BSS
    lea __bss_end(%rip), %rsi        # End of BSS  
    
    DBG 's'  # Debug: loaded symbols
    
    # Calculate BSS size in bytes
    mov %rsi, %rcx
    sub %rdi, %rcx                   # Size = end - start
    
    DBG 'z'  # Debug: calculated size
    
    # Check if there's anything to clear
    test %rcx, %rcx
    jz .bss_safe_done               # Skip if size is 0
    
    # Linux-style: Add 3 for rounding, then divide by 4 (32-bit alignment)
    add $3, %rcx                    # Round up for 4-byte alignment
    shr $2, %rcx                    # Divide by 4 (number of dwords)
    
    DBG 'r'  # Debug: about to clear
    
    # Clear BSS using rep stosl (store long-word, 32-bit) like Linux
    xor %eax, %eax                  # Fill value: 0
    rep stosl                       # Clear RCX dwords starting at RDI
    
    DBG 'd'  # Debug: cleared BSS
    
.bss_safe_done:
    DBG 'e'  # Debug: BSS done
    ret

# Keep old function for reference
clear_bss:
    DBG 'b'  # Debug: entered clear_bss
    
    # Load BSS start and end addresses
    lea __bss_start(%rip), %rdi      # Destination: start of BSS
    lea __bss_end(%rip), %rcx        # End of BSS
    
    DBG 's'  # Debug: loaded symbols
    
    # Calculate BSS size
    sub %rdi, %rcx                   # Size = end - start
    
    DBG 'z'  # Debug: calculated size
    
    # Check if there's anything to clear
    test %rcx, %rcx
    jz .bss_done                     # Skip if size is 0
    
    DBG 'r'  # Debug: about to clear
    
    # Clear BSS using rep stosb (store byte)
    xor %eax, %eax                   # Fill value: 0
    rep stosb                        # Clear RCX bytes starting at RDI
    
    DBG 'd'  # Debug: cleared BSS
    
.bss_done:
    DBG 'e'  # Debug: BSS done
    ret

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
    # Debug: Print 'F' for page fault
    mov $0x3f8, %dx
    mov $'F', %al
    out %al, %dx
    
    # Get CR2 (faulting address) and print it
    mov %cr2, %rax
    # Simple hex dump - just print a few chars to indicate fault
    mov $0x3f8, %dx
    mov $'A', %al
    out %al, %dx
    mov $'U', %al  
    out %al, %dx
    mov $'L', %al
    out %al, %dx
    mov $'T', %al
    out %al, %dx
    
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

# Kernel stack is now defined in linker script within BSS section