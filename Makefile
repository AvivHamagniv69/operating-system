BOOT_FOLDER := src/impl/boot
LINKER_FOLDER := targets
KERNEL_BIN_FOLDER := targets/iso/boot

NASM_FLAGS := -f elf32
OBJECT_FILES := header.o main.o
LINKER_FLAGS := -m elf_i386

.PHONY: default
default: $(KERNEL_BIN_FOLDER)/kernel.bin
	grub-mkrescue /usr/lib/grub/i386-pc -o kernel.iso targets/iso

$(KERNEL_BIN_FOLDER)/kernel.bin: $(OBJECT_FILES)
	ld $(LINKER_FLAGS) -T $(LINKER_FOLDER)/linker.ld -o $@ $^

%.o: $(BOOT_FOLDER)/%.asm
	nasm $(NASM_FLAGS) -o $@ $<

.PHONY: clean
clean:
	rm *.o
	rm *.bin
	rm *.iso

.PHONY: run
run:
	qemu-system-i386 -cdrom kernel.iso