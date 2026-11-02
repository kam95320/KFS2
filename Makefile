CC = gcc
LD = ld
NASM = nasm
BUILD = build
KERNEL = $(BUILD)/kernel.elf
ISO = $(BUILD)/kfs2.iso
CFLAGS = -m32 -march=i386 -std=c99 -Wall -Wextra -Werror \
	-ffreestanding -fno-stack-protector -fno-builtin \
	-nostdlib -nodefaultlibs -fno-pie -fno-pic \
	-fno-asynchronous-unwind-tables -fno-unwind-tables -MMD -MP
SOURCES = src/kernel.c src/vga.c src/gdt.c src/stack.c
OBJECTS = $(BUILD)/boot.o $(SOURCES:src/%.c=$(BUILD)/%.o)

.PHONY: all clean fclean re run check
all: $(ISO)

$(BUILD):
	mkdir -p $@

$(BUILD)/boot.o: src/boot.asm | $(BUILD)
	$(NASM) -f elf32 $< -o $@

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL): $(OBJECTS) linker.ld
	$(LD) -m elf_i386 -T linker.ld -o $@ $(OBJECTS)
	test "$$(wc -c < $@)" -lt 10000000

$(ISO): $(KERNEL) boot/grub/grub.cfg
	mkdir -p $(BUILD)/iso/boot/grub
	cp $(KERNEL) $(BUILD)/iso/boot/kernel.elf
	cp boot/grub/grub.cfg $(BUILD)/iso/boot/grub/grub.cfg
	grub-mkrescue -o $@ $(BUILD)/iso

check: all
	grub-file --is-x86-multiboot $(KERNEL)
	test -z "$$(nm -u $(KERNEL))"
	test "$$(wc -c < $(ISO))" -lt 10000000
	python3 tests/check_boot.py $(abspath $(ISO)) $(abspath $(KERNEL)) $(abspath $(BUILD)/checks)

clean:
	rm -rf $(BUILD) build-lab

fclean: clean
	rm -f kernel.elf kfs1.iso

re: fclean
	$(MAKE) all

run: all
	qemu-system-i386 -cdrom $(ISO)

-include $(OBJECTS:.o=.d)
