# Myria OS Build Helpers
.PHONY: clean iso qemu qemu-debug setup-limine build rebuild run rebuild-run

# Build directories
BUILD_DIR = build
ISO_DIR = $(BUILD_DIR)/iso
LIMINE_DIR = $(BUILD_DIR)/limine

# QEMU configuration
QEMU = qemu-system-x86_64
QEMU_FLAGS = -cpu max -smp 2 -m 512M \
	-cdrom $(BUILD_DIR)/myria.iso \
	-serial stdio -no-reboot -display none

# Default target
all: iso

# Build kernel
build: build/kernel/kernel.elf

# Force rebuild kernel
rebuild: clean build iso


# Configure and build kernel with CMake
build/kernel/kernel.elf:
	@echo "Configuring CMake build..."
	@cmake -DCMAKE_TOOLCHAIN_FILE=toolchain-x86_64.cmake -B build
	@echo "Building kernel..."
	@cmake --build build

# Create bootable ISO
iso: $(BUILD_DIR)/myria.iso

$(BUILD_DIR)/myria.iso: build/kernel/kernel.elf setup-limine
	@echo "Creating ISO structure..."
	@mkdir -p $(ISO_DIR)
	@mkdir -p $(ISO_DIR)/boot
	@mkdir -p $(ISO_DIR)/EFI/BOOT
	@cp build/kernel/kernel.elf $(ISO_DIR)/kernel.elf
	@cp build/kernel/kernel.elf $(ISO_DIR)/boot/kernel.elf
	@cp boot/limine.cfg $(ISO_DIR)/limine.cfg
	@cp boot/limine.cfg $(ISO_DIR)/boot/limine.cfg
	@cp $(LIMINE_DIR)/limine-bios.sys $(LIMINE_DIR)/limine-bios-cd.bin $(ISO_DIR)/
	@cp $(LIMINE_DIR)/limine-uefi-cd.bin $(ISO_DIR)/
	@cp $(LIMINE_DIR)/BOOTX64.EFI $(ISO_DIR)/EFI/BOOT/
	
	@echo "Creating ISO image..."
	@xorriso -as mkisofs \
		-b limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		$(ISO_DIR) -o $(BUILD_DIR)/myria.iso 2>/dev/null
	
	@$(LIMINE_DIR)/limine bios-install $(BUILD_DIR)/myria.iso 2>/dev/null
	@echo "ISO created: $(BUILD_DIR)/myria.iso"

# Set up Limine bootloader
setup-limine:
	@if [ ! -d "$(LIMINE_DIR)" ]; then \
		echo "Downloading Limine..."; \
		mkdir -p $(BUILD_DIR); \
		git clone https://github.com/limine-bootloader/limine.git --branch=v7.x-binary --depth=1 $(LIMINE_DIR); \
		$(MAKE) -C $(LIMINE_DIR); \
	fi

# Run in QEMU
qemu: iso
	@echo "Starting QEMU..."
	$(QEMU) $(QEMU_FLAGS)

# Alias for run
run: qemu

# Force rebuild and run
rebuild-run: rebuild iso
	@echo "Starting QEMU with rebuilt kernel..."
	$(QEMU) $(QEMU_FLAGS)

# Run in QEMU with debug
qemu-debug: iso
	@echo "Starting QEMU with debug..."
	$(QEMU) $(QEMU_FLAGS) -s -S -d int,cpu_reset

# Run QEMU with full logging to files
qemu-log: iso
	@echo "Starting QEMU with full logging..."
	@mkdir -p logs
	$(QEMU) $(QEMU_FLAGS) \
		-D logs/qemu.log \
		-d int,cpu_reset,exec,in_asm \
		-monitor file:logs/monitor.log \
		-serial file:logs/serial.log

# Clean build artifacts
clean:
	@echo "Cleaning build directory..."
	@rm -rf $(BUILD_DIR)/*

# Clean logs
clean-logs:
	@echo "Cleaning log files..."
	@rm -rf logs

# Help target
help:
	@echo "Myria OS Build System"
	@echo ""
	@echo "Targets:"
	@echo "  iso        - Create bootable ISO image"
	@echo "  qemu       - Run in QEMU"
	@echo "  qemu-debug - Run in QEMU with debug flags (-s -S)"
	@echo "  qemu-log   - Run in QEMU with full logging to logs/ directory"
	@echo "  clean      - Clean build artifacts"
	@echo "  clean-logs - Clean log files"
	@echo "  help       - Show this help message"
	@echo ""
	@echo "Prerequisites:"
	@echo "  - x86_64-elf-gcc or clang cross-compiler"
	@echo "  - xorriso for ISO creation"
	@echo "  - QEMU with x86_64 support"
	@echo "  - OVMF.fd UEFI firmware in PATH or current directory"