BOOT_FOLDER := src/impl/boot
LINKER_FOLDER := targets
KERNEL_BIN_FOLDER := targets/iso/boot
KERNEL_FOLDER := src/impl/kernel

NASM_FLAGS := -f elf32
GCC_FLAGS := -Wall
OBJECT_FILES := header.o main.o main64.o kmain.o print.o 
LINKER_FLAGS := -m elf_i386

.PHONY: default
default: $(KERNEL_BIN_FOLDER)/kernel.bin
	grub-mkrescue /usr/lib/grub/i386-pc -o kernel.iso targets/iso

$(KERNEL_BIN_FOLDER)/kernel.bin: $(OBJECT_FILES)
	ld $(LINKER_FLAGS) -T $(LINKER_FOLDER)/linker.ld -o $@ $^

%.o: $(BOOT_FOLDER)/%.asm
	nasm $(NASM_FLAGS) -o $@ $<

kmain.o: $(KERNEL_FOLDER)/kmain.c $(KERNEL_FOLDER)/print.o
	gcc $(GCC_FLAGS) -o $@ -c $^

%.o: $(KERNEL_FOLDER)/%.c
	gcc $(GCC_FLAGS) -o $@ -c $<

.PHONY: clean
clean:
	rm *.o
	rm $(KERNEL_BIN_FOLDER)/*.bin
	rm *.iso

.PHONY: run
run:
	qemu-system-x86_64 -cdrom kernel.iso
