gcc_options = -m32 -fno-stack-protector -fno-builtin -Wall -std=c11
nasm_options = -f elf32
linker_options = -m elf_i386

default: kernel
	mv kernel os/boot/kernel
	grub-mkrescue -o os.iso os/

kernel: boot.o kernel.o vga.o
	ld -m elf_i386 -T linker.ld -o kernel boot.o kernel.o vga.o

kernel.o:
	gcc $(gcc_options) -c kernel.c -o kernel.o

vga.o:
	gcc $(gcc_options) -c vga.c -o vga.o

boot.o:
	nasm $(nasm_options) boot.s -o boot.o

run:
	qemu-system-i386 os.iso

clean:
	rm kernel.o vga.o boot.o os/boot/kernel os.iso