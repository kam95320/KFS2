.PHONY: all clean fclean re run

all: kfs1.iso

build-lab/boot.o: src/boot.asm
	mkdir -p build-lab
	nasm -f elf32 src/boot.asm -o build-lab/boot.o

build-lab/kernel.o: src/kernel.c
	mkdir -p build-lab
	gcc -m32 -ffreestanding -fno-stack-protector -fno-builtin \
		-nostdlib -nodefaultlibs -fno-pie -fno-pic \
		-fno-asynchronous-unwind-tables -fno-unwind-tables \
		-c src/kernel.c -o build-lab/kernel.o

kernel.elf: build-lab/boot.o build-lab/kernel.o linker.ld
	ld -m elf_i386 -T linker.ld \
		-o kernel.elf \
		build-lab/boot.o build-lab/kernel.o

kfs1.iso: kernel.elf boot/grub/grub.cfg
	mkdir -p build-lab/iso/boot/grub
	cp kernel.elf build-lab/iso/boot/kernel.elf
	cp boot/grub/grub.cfg build-lab/iso/boot/grub/grub.cfg
	grub-mkrescue -o kfs1.iso build-lab/iso

clean:
	rm -rf build-lab

fclean: clean
	rm -f kernel.elf kfs1.iso

re: fclean
	$(MAKE) all

run: all
	qemu-system-i386 -cdrom kfs1.iso
