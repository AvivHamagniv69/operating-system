BOOT_SOURCE_FOLDER := src/impl/boot
LINKER_FOLDER := targets
KERNEL_BIN_FOLDER := targets/iso/boot
KERNEL_SOURCE_FOLDER := src/impl/kernel
OBJECT_FILES_FOLDER := build/object_files

NASM_FLAGS := -f elf64
GCC_FLAGS := -Wall -ffreestanding -std=c11
LINKER_FLAGS := 

C_SOURCE_FILES := $(shell find $(KERNEL_SOURCE_FOLDER) -name *.c)
C_OBJECT_FILES := $(patsubst $(KERNEL_SOURCE_FOLDER)/%.c, $(OBJECT_FILES_FOLDER)/%.o, $(C_SOURCE_FILES))

ASM_SOURCE_FILES := $(shell find $(BOOT_SOURCE_FOLDER) -name *.asm)
ASM_OBJECT_FILES := $(patsubst $(BOOT_SOURCE_FOLDER)/%.asm, $(OBJECT_FILES_FOLDER)/%.o, $(ASM_SOURCE_FILES))

OBJECT_FILES := $(C_OBJECT_FILES) $(ASM_OBJECT_FILES)

GRUB_MKRESCUE_ARCHITECHTURE := /usr/lib/grub/i386-pc

.PHONY: default
default: $(KERNEL_BIN_FOLDER)/kernel.bin
	grub-mkrescue $(GRUB_MKRESCUE_ARCHITECHTURE) -o kernel.iso targets/iso

$(KERNEL_BIN_FOLDER)/kernel.bin: $(OBJECT_FILES)
	ld $(LINKER_FLAGS) -T $(LINKER_FOLDER)/linker.ld -o $@ $^

$(OBJECT_FILES_FOLDER)/%.o: $(BOOT_SOURCE_FOLDER)/%.asm
	nasm $(NASM_FLAGS) -o $@ $<

$(OBJECT_FILES_FOLDER)/%.o: $(KERNEL_SOURCE_FOLDER)/%.c
	gcc $(GCC_FLAGS) -o $@ -c $<

.PHONY: clean
clean:
	rm $(OBJECT_FILES_FOLDER)/*.o
	rm $(KERNEL_BIN_FOLDER)/*.bin
	rm *.iso

.PHONY: run
run:
	qemu-system-x86_64 -cdrom kernel.iso