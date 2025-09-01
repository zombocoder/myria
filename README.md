# Myria OS

A minimal x86-64 operating system kernel written in C11 and assembly, designed for live process migration and distributed computing.

## Status

🎉 **M2 MILESTONE COMPLETE** - Full user/kernel separation with working syscalls

✅ **M1 Milestone Complete** - Basic memory management and kernel infrastructure  
✅ **M0 Milestone Complete** - Boot sequence and serial I/O

### Working Components

#### Core Infrastructure
- ✅ **Boot Process** - UEFI + Limine bootloader with 64-bit long mode handoff
- ✅ **Enhanced Linker Script** - W^X memory protection, proper section alignment
- ✅ **Serial I/O** - Console output and comprehensive debugging
- ✅ **Exception Handling** - Per-vector IDT with detailed fault analysis

#### Memory Management  
- ✅ **PMM (Physical Memory Manager)** - Enhanced page frame allocator with statistics
- ✅ **VMM (Virtual Memory Manager)** - Complete virtual memory with heap management
- ✅ **Page Table Isolation** - Separate user PML4 with kernel high-half sharing
- ✅ **Memory Protection** - U=1 permissions at ALL page table levels
- ✅ **TLB Management** - Proper cache coherency and invalidation

#### User/Kernel Separation
- ✅ **GDT/TSS Setup** - Complete segment descriptors for privilege separation
- ✅ **IRETQ User Entry** - Clean ring 0 → ring 3 transitions
- ✅ **SYSCALL/SYSRET** - Fast system call interface with proper register handling
- ✅ **Isolated Address Spaces** - Hardware-enforced memory isolation
- ✅ **User Mode Execution** - Full user program execution with syscalls

#### Threading and Scheduling
- ✅ **Scheduler** - Basic cooperative threading infrastructure  
- ✅ **Thread Management** - Thread creation, state tracking, and cleanup
- ✅ **Context Switching** - Basic thread switching (cooperative)

## Build Requirements

- **Cross-compiler**: x86_64-elf-gcc or clang 
- **Build system**: CMake + GNU Make
- **ISO creation**: xorriso
- **Emulation**: QEMU with x86_64 support
- **Bootloader**: Limine (automatically downloaded)

### macOS Setup
```bash
# Install cross-compiler toolchain
brew install x86_64-elf-gcc
brew install cmake
brew install xorriso
brew install qemu
```

## Building and Running

```bash
# Build the kernel and create ISO
make clean && make

# Run in QEMU
make qemu

# Run with debug output
make qemu-debug

# Run with full logging
make qemu-log
```

## Architecture

### Memory Layout
- **Kernel VA**: `0xffffffff80000000` - `0xffffffff9fffffff` (Higher-half kernel)
- **Kernel heap**: `0xffffffffA0000000` - `0xffffffffBfffffff`
- **Physical mapping**: `0xffffffffC0000000` - `0xffffffffDfffffff` (HHDM)  
- **User code**: `0x10000` (Low canonical addresses, isolated PML4)
- **User stack**: `0x800000-0x802000` (8KB stack with guard pages)

### Enhanced Boot Sequence
1. **UEFI firmware** loads Limine bootloader from ISO
2. **Limine** loads kernel ELF and sets up initial 4-level paging
3. **Kernel entry** (`start.S`) initializes x86-64 long mode environment
4. **Page table setup** - Kernel writable sections with proper R/W/X permissions
5. **GDT/TSS initialization** - Segment descriptors for user/kernel separation
6. **IDT initialization** - Per-vector exception handlers with fault analysis
7. **PMM initialization** - Physical memory management with statistics
8. **VMM initialization** - Virtual memory, heap, and page table isolation
9. **Kernel PML4 template** - Template for user address space creation
10. **Scheduler initialization** - Threading infrastructure and context switching
11. **Syscall initialization** - SYSCALL/SYSRET MSRs and dispatch framework
12. **User mode test** - Create isolated user process and execute syscalls
13. **System ready** - All major components operational

## Key Files

```
/kernel/
├── arch/x86_64/              # x86-64 architecture support
│   ├── start.S               # Kernel entry point and early boot
│   ├── linker.ld             # Enhanced linker script with W^X
│   ├── gdt.c                 # GDT/TSS setup and user mode entry
│   ├── gdt_flush.S           # GDT loading assembly
│   ├── idt_minimal.c         # Per-vector IDT with fault analysis
│   ├── msr.c                 # MSR management for syscalls
│   ├── syscall_entry.S       # SYSCALL/SYSRET assembly handler
│   ├── paging.c              # Page table setup and management
│   └── portio.c              # Port I/O utilities
├── mm/                       # Memory management subsystem
│   ├── pmm_simple.c          # Physical memory manager with stats
│   ├── vmm.c                 # Virtual memory manager with heap
│   ├── user_mapping.c        # User page mapping utilities
│   ├── user_as.c             # Isolated user address space creation
│   └── paging.c              # Page table manipulation
├── sched/                    # Threading and scheduling
│   └── thread_minimal.c      # Cooperative threading with statistics
├── syscall/                  # System call interface
│   └── syscalls.c            # Syscall dispatch and implementations
├── util/                     # Kernel utilities
│   └── serial.c              # Serial console I/O and debugging
├── minimal_test.c            # Main kernel tests and initialization
├── user_init.c               # User mode initialization and testing
└── user_payload.S            # User mode test program (assembly)
```

## Development Notes

### Technical Achievements
- **Complete User/Kernel Isolation** - Hardware-enforced separation using x86-64 privilege levels
- **Advanced Memory Protection** - W^X (Write XOR Execute) memory regions with proper page permissions
- **Fast System Calls** - SYSCALL/SYSRET instructions for sub-microsecond kernel transitions
- **TLB Coherency Management** - Solved complex paging-structure cache issues with isolated PML4 approach
- **Position-Independent User Code** - Eliminates virtual address conflicts and enables flexible loading

### M2 Boot Output
```
X86_EARLY_INIT WORKING!
Setting up kernel page tables with proper R/W/X permissions
...
=== Enhanced Memory Management Test ===
[PMM] Memory statistics available
[HEAP] Multiple allocations succeeded!
=== Enhanced Threading System Test ===
[TEST] All threads created successfully
=== System Call Interface Test ===  
[SYSCALL] Received syscall number: 1 (WRITE)
=== User Mode Test (Isolated Address Space) ===
[USER] User process created with isolated address space
[USER] Code mapped at 0x10000 (RX, U=1)
[USER] Stack mapped at 0x800000-0x802000 (RW, NX, U=1)
[GDT] Stack aligned, executing IRETQ...
[SYSCALL] Received syscall number: 0 (EXIT)
[SYSCALL] sys_exit called with code 99 - SUCCESS!
=== M2 MILESTONE COMPLETE ===
✓ User/kernel separation working
✓ Page table isolation working  
✓ IRETQ user mode entry working
✓ SYSCALL/SYSRET interface working
```

## Future Development

### M3 Milestone - Advanced Process Management
1. **Process Creation/Destruction** - Complete process lifecycle management
2. **Preemptive Scheduling** - Timer-based multitasking with priority queues  
3. **Advanced Syscalls** - File I/O, process control, signal handling
4. **VFS Layer** - Virtual file system with device driver integration
5. **ELF Loading** - Dynamic user program loading from disk/network

### M4 Milestone - Device Drivers and I/O
1. **Storage Drivers** - NVMe/SATA for persistent storage
2. **Network Drivers** - Ethernet with TCP/IP stack
3. **Input Drivers** - Keyboard, mouse, and console management
4. **Device Management** - Unified device tree and resource allocation
5. **Interrupt Handling** - Advanced APIC configuration and IRQ routing

### Long-term Vision (M5-M6)
- **Live Process Migration** - Checkpoint/restore with minimal downtime
- **Distributed Computing** - Multi-node process execution and load balancing
- **VIP Management** - Virtual IP addresses with transparent failover
- **Capability-based Security** - Fine-grained permissions and isolation
- **Post-copy Migration** - Advanced migration with demand paging

## Debugging

### QEMU Debugging
```bash
# Run with GDB support
make qemu-debug
# In another terminal:
gdb build/kernel/kernel.elf
(gdb) target remote :1234
(gdb) continue
```

### Serial Logging
All kernel output goes to serial port (COM1), visible in QEMU console when using `-serial stdio`.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.