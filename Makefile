ASM = nasm
CC = i686-elf-gcc
LD = i686-elf-ld

ASMFLAGS = -f elf32
CFLAGS = -m32 -march=i686 -ffreestanding -O2 -Wall -Wextra -fno-exceptions -fno-rtti -nostdlib -nostdinc -I$(INCLUDE_DIR) -fno-stack-protector
LDFLAGS = -T linker.ld -nostdlib

BOOT_DIR = boot
KERNEL_DIR = kernel
ARCH_DIR = $(KERNEL_DIR)/arch/i386
LIB_DIR = $(KERNEL_DIR)/lib
DRIVERS_DIR = $(KERNEL_DIR)/drivers
SYSCALLS_DIR = $(KERNEL_DIR)/syscalls
PROCESS_DIR = $(KERNEL_DIR)/process
INCLUDE_DIR = include

BOOT_STAGE1 = $(BOOT_DIR)/stage1.asm
BOOT_STAGE2 = $(BOOT_DIR)/stage2.asm

KERNEL_ENTRY = $(KERNEL_DIR)/kernel_entry.asm
KERNEL_ASM_SRC = $(ARCH_DIR)/gdt_flush.asm \
                 $(ARCH_DIR)/idt_flush.asm \
                 $(ARCH_DIR)/paging_flush.asm \
                 $(ARCH_DIR)/tss_flush.asm \
                 $(ARCH_DIR)/syscall.asm \
                 $(ARCH_DIR)/usermode.asm

KERNEL_C_SRC = $(KERNEL_DIR)/kernel.cpp \
               $(ARCH_DIR)/gdt.cpp \
               $(ARCH_DIR)/idt.cpp \
               $(ARCH_DIR)/tss.cpp \
               $(LIB_DIR)/memory.cpp \
               $(KERNEL_DIR)/shell.cpp \
               $(KERNEL_DIR)/mm/heap.cpp \
               $(KERNEL_DIR)/mm/paging.cpp \
               $(KERNEL_DIR)/panic.cpp \
               $(KERNEL_DIR)/editor.cpp \
               $(SYSCALLS_DIR)/syscall.cpp \
               $(PROCESS_DIR)/process.cpp \
               $(PROCESS_DIR)/scheduler.cpp \
               $(PROCESS_DIR)/elf.cpp \
               $(DRIVERS_DIR)/vga.cpp \
               $(DRIVERS_DIR)/keyboard.cpp \
               $(DRIVERS_DIR)/timer.cpp \
               $(DRIVERS_DIR)/ata.cpp \
               $(DRIVERS_DIR)/fat32.cpp \
               $(DRIVERS_DIR)/speaker.cpp

KERNEL_ENTRY_OBJ = $(KERNEL_ENTRY:.asm=.o)
KERNEL_ASM_OBJ = $(KERNEL_ASM_SRC:.asm=.o)
KERNEL_C_OBJ = $(KERNEL_C_SRC:.cpp=.o)

STAGE1_BIN = stage1.bin
STAGE2_BIN = stage2.bin
KERNEL_BIN = kernel.bin
OS_IMAGE = orangeos.img

USER_TEST_SRC = userspace/testes/hello.c
USER_PROG = hello.elf

LIBC_SRC = userspace/libc/stdio.c userspace/libc/string.c
LIBC_ASM = userspace/libc/syscalls.asm
LIBC_OBJ = $(LIBC_SRC:.c=.o) $(LIBC_ASM:.asm=.o)

all: generate_version $(KERNEL_BIN) $(STAGE1_BIN) $(STAGE2_BIN) $(USER_PROG) update_image

generate_version:
	@bash scripts/increment_build.sh

PART_IMG = partition.img
MBR_TEMP = mbr_temp.bin

# New target to create the image from scratch (run 'make image' once)
image: $(STAGE1_BIN) $(STAGE2_BIN) $(KERNEL_BIN) $(USER_PROG)
	@echo "Creating NEW disk image with MBR and FAT32 partition..."
	@dd if=/dev/zero of=$(OS_IMAGE) bs=1M count=50 2>/dev/null || dd if=/dev/zero of=$(OS_IMAGE) bs=1024 count=51200 2>/dev/null || (echo "Error: Could not create disk image" && exit 1)
	@dd if=$(STAGE1_BIN) of=$(OS_IMAGE) bs=512 count=1 conv=notrunc 2>/dev/null
	@echo "Writing partition table and boot signature..."
	@if command -v python3 >/dev/null 2>&1; then python3 -c "f=open('$(OS_IMAGE)','r+b'); f.seek(446); f.write(bytes([0x80,0x01,0x01,0x00,0x0C,0xFF,0xFF,0xFF,0x00,0x08,0x00,0x00,0x00,0x88,0x01,0x00])); f.seek(510); f.write(bytes([0x55,0xAA])); f.close(); print('Partition table written')"; elif command -v perl >/dev/null 2>&1; then perl -e 'open(F,"+<","$(OS_IMAGE)"); seek(F,446,0); print F pack("C*",0x80,0x01,0x01,0x00,0x0C,0xFF,0xFF,0xFF,0x00,0x08,0x00,0x00,0x00,0x88,0x01,0x00); seek(F,510,0); print F pack("C*",0x55,0xAA); close(F); print "Partition table written\n"'; else echo "Warning: Need python3 or perl to write partition table. Using fallback method."; echo -ne '\x80\x01\x01\x00\x0C\xFF\xFF\xFF\x00\x08\x00\x00\x00\x88\x01\x00' | dd of=$(OS_IMAGE) bs=1 seek=446 conv=notrunc 2>/dev/null; echo -ne '\x55\xAA' | dd of=$(OS_IMAGE) bs=1 seek=510 conv=notrunc 2>/dev/null; fi
	@dd if=$(STAGE2_BIN) of=$(OS_IMAGE) bs=512 seek=1 count=10 conv=notrunc 2>/dev/null
	@dd if=$(KERNEL_BIN) of=$(OS_IMAGE) bs=512 seek=12 conv=notrunc 2>/dev/null
	@echo "Formatting FAT32 partition (49MB, should take a few seconds)..."
	@PART_START=1048576; PART_SIZE=51380224; dd if=$(OS_IMAGE) of=$(PART_IMG) bs=1M skip=1 count=49 2>/dev/null || dd if=$(OS_IMAGE) of=$(PART_IMG) bs=1 skip=$$PART_START count=$$PART_SIZE 2>/dev/null; if command -v mkfs.fat >/dev/null 2>&1; then echo "  Running mkfs.fat..."; mkfs.fat -F 32 -n "ORANGEOS" -s 1 $(PART_IMG) 2>&1 | head -1 || mkfs.fat -F 32 -n "ORANGEOS" $(PART_IMG) 2>&1 | head -1 || true; echo "  Formatting complete."; else echo "  Warning: mkfs.fat not found, skipping FAT32 formatting"; fi; dd if=$(PART_IMG) of=$(OS_IMAGE) bs=1M seek=1 conv=notrunc 2>/dev/null || dd if=$(PART_IMG) of=$(OS_IMAGE) bs=1 seek=$$PART_START conv=notrunc 2>/dev/null; rm -f $(PART_IMG) 2>/dev/null || true
	@echo "Restoring boot signature after formatting..."
	@if command -v python3 >/dev/null 2>&1; then python3 -c "f=open('$(OS_IMAGE)','r+b'); f.seek(510); f.write(bytes([0x55,0xAA])); f.close(); print('  Boot signature restored')"; elif command -v perl >/dev/null 2>&1; then perl -e 'open(F,"+<","$(OS_IMAGE)"); seek(F,510,0); print F pack("C*",0x55,0xAA); close(F); print "  Boot signature restored\n"'; else echo -ne '\x55\xAA' | dd of=$(OS_IMAGE) bs=1 seek=510 conv=notrunc 2>/dev/null && echo "  Boot signature restored (using echo)"; fi
	@rm -f $(MBR_TEMP) 2>/dev/null || true
	@echo "Copying userspace programs to disk image..."
	@if command -v mcopy >/dev/null 2>&1; then mcopy -i $(OS_IMAGE)@@1M $(USER_PROG) ::/ || echo "Warning: mcopy failed"; else echo "Warning: mtools not found. Cannot copy userspace programs to disk image."; fi
	@echo "Build complete: $(OS_IMAGE)"

# Target to update existing image without formatting
update_image: $(KERNEL_BIN) $(STAGE1_BIN) $(STAGE2_BIN) $(USER_PROG)
	@if [ ! -f $(OS_IMAGE) ]; then echo "Error: $(OS_IMAGE) not found. Run 'make image' first!"; exit 1; fi
	@echo "Updating kernel and bootloader on EXISTING image..."
	@dd if=$(STAGE1_BIN) of=$(OS_IMAGE) bs=512 count=1 conv=notrunc 2>/dev/null
	@dd if=$(STAGE2_BIN) of=$(OS_IMAGE) bs=512 seek=1 count=10 conv=notrunc 2>/dev/null
	@dd if=$(KERNEL_BIN) of=$(OS_IMAGE) bs=512 seek=12 conv=notrunc 2>/dev/null
	@echo "Updating userspace programs..."
	@if command -v mcopy >/dev/null 2>&1; then mcopy -o -i $(OS_IMAGE)@@1M $(USER_PROG) ::/ || echo "Warning: mcopy failed"; else echo "Warning: mtools not found. Cannot copy userspace programs to disk image."; fi
	@echo "Update complete."

$(STAGE1_BIN): $(BOOT_STAGE1)
	@echo "Assembling Stage 1..."
	$(ASM) -f bin $(BOOT_STAGE1) -o $(STAGE1_BIN)

$(STAGE2_BIN): $(BOOT_STAGE2)
	@echo "Assembling Stage 2..."
	$(ASM) -f bin $(BOOT_STAGE2) -o $(STAGE2_BIN)

$(KERNEL_BIN): $(KERNEL_ENTRY_OBJ) $(KERNEL_ASM_OBJ) $(KERNEL_C_OBJ)
	@echo "Linking kernel..."
	$(LD) $(LDFLAGS) -o kernel.elf $(KERNEL_ENTRY_OBJ) $(KERNEL_ASM_OBJ) $(KERNEL_C_OBJ)
	objcopy -O binary kernel.elf $(KERNEL_BIN)

%.o: %.asm
	@echo "Assembling $<..."
	$(ASM) $(ASMFLAGS) $< -o $@

%.o: %.cpp
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Userspace rules
userspace/libc/%.o: userspace/libc/%.c
	@echo "Compiling libc $<..."
	$(CC) -m32 -ffreestanding -nostdlib -fno-stack-protector -Iuserspace/include -c $< -o $@

userspace/libc/%.o: userspace/libc/%.asm
	@echo "Assembling libc $<..."
	$(ASM) -f elf32 $< -o $@

$(USER_PROG): $(USER_TEST_SRC) $(LIBC_OBJ)
	@echo "Linking userspace program $(USER_PROG)..."
	$(CC) -m32 -ffreestanding -nostdlib -fno-stack-protector -Iuserspace/include -c $(USER_TEST_SRC) -o userspace/testes/hello.o
	$(LD) -m elf_i386 -T userspace/linker.ld -o $(USER_PROG) userspace/testes/hello.o $(LIBC_OBJ)

clean:
	rm -f $(KERNEL_ENTRY_OBJ) $(KERNEL_ASM_OBJ) $(KERNEL_C_OBJ)
	rm -f $(STAGE1_BIN) $(STAGE2_BIN) $(KERNEL_BIN)
	rm -f kernel.elf $(OS_IMAGE) $(PART_IMG) $(MBR_TEMP)
	rm -f $(USER_PROG) userspace/testes/*.o userspace/libc/*.o

run: $(OS_IMAGE)
	qemu-system-i386 -drive format=raw,file=$(OS_IMAGE),if=ide,index=0,media=disk -m 32M -boot c -machine acpi=off

debug: $(OS_IMAGE)
	qemu-system-i386 -drive format=raw,file=$(OS_IMAGE),if=ide,index=0,media=disk -m 32M -boot c -machine acpi=off -s -S

verify: $(OS_IMAGE)
	@echo "Verifying bootloader..."
	@if [ -f $(OS_IMAGE) ]; then echo "Checking boot signature at offset 510..."; hexdump -C $(OS_IMAGE) -s 510 -n 2 | grep -q "55 aa" && echo "  [OK] Boot signature found" || echo "  [FAIL] Boot signature missing!"; echo "Checking first bytes of bootloader..."; hexdump -C $(OS_IMAGE) -n 32; echo "Checking partition table (offset 446-461)..."; hexdump -C $(OS_IMAGE) -s 446 -n 16; else echo "Error: $(OS_IMAGE) not found. Run 'make all' first."; fi

.PHONY: all clean run debug verify