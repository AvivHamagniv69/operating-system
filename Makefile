BOOT_SOURCE_FOLDER := src/impl/boot
LINKER_FOLDER := targets
KERNEL_BIN_FOLDER := targets/iso/boot
KERNEL_SOURCE_FOLDER := src/impl/kernel
OBJECT_FILES_FOLDER := build/object_files
IMPL_FOLDER := src/impl
IMPL_SUB_FOLDERS := $(sort $(dir $(wildcard src/impl/*/)))

NASM_FLAGS := -f elf32
GCC_FLAGS := -g -m32 -Wall -Wextra -ffreestanding -fno-builtin -std=c11
LINKER_FLAGS := -m elf_i386

C_SOURCE_FILES := $(shell find $(IMPL_FOLDER)/*/ -name *.c)
C_OBJECT_FILES := $(patsubst %.c, $(OBJECT_FILES_FOLDER)/%.o, $(notdir $(C_SOURCE_FILES)))

ASM_SOURCE_FILES := $(shell find $(IMPL_FOLDER)/*/ -name *.asm)
ASM_OBJECT_FILES := $(patsubst %.asm, $(OBJECT_FILES_FOLDER)/%.o, $(notdir $(ASM_SOURCE_FILES)))

OBJECT_FILES := $(C_OBJECT_FILES) $(ASM_OBJECT_FILES)
$(info $(C_SOURCE_FILES))
$(info $(ASM_SOURCE_FILES))
$(info $(OBJECT_FILES))

GRUB_MKRESCUE_ARCHITECHTURE := /usr/lib/grub/i386-pc

.PHONY: default
default: $(KERNEL_BIN_FOLDER)/kernel.bin
	grub-mkrescue $(GRUB_MKRESCUE_ARCHITECHTURE) -o kernel.iso targets/iso
	rm $(OBJECT_FILES_FOLDER)/*.o

$(KERNEL_BIN_FOLDER)/kernel.bin: $(OBJECT_FILES)
	ld $(LINKER_FLAGS) -T $(LINKER_FOLDER)/linker.ld -o $@ $^

$(OBJECT_FILES_FOLDER)/%.o: $(IMPL_FOLDER)/*/%.asm
	nasm $(NASM_FLAGS) -o $@ $<

$(OBJECT_FILES_FOLDER)/%.o: $(IMPL_FOLDER)/*/%.c
	/home/aviv/opt/cross/bin/i686-elf-gcc $(GCC_FLAGS) -o $@ -c $<

.PHONY: clean
clean:
	rm $(KERNEL_BIN_FOLDER)/*.bin
	rm *.iso

.PHONY: run
run:
	qemu-system-i386 -cdrom kernel.iso

.PHONY: run_debug
run_debug:
	qemu-system-i386 -s -S -cdrom kernel.iso