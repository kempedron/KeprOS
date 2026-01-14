# Makefile для KeprOS

CC = gcc
LD = ld
ASM = nasm

CFLAGS = -m32 -ffreestanding -nostdlib -fno-builtin -fno-stack-protector
LDFLAGS = -m elf_i386 -T link.ld -nostdlib
ASFLAGS = -f elf32

OBJECTS = boot.o kernel.o

.PHONY: all clean run

all: kernel
# Makefile для KeprOS

CC = gcc
LD = ld
ASM = nasm

CFLAGS = -m32 -ffreestanding -nostdlib -fno-builtin -fno-stack-protector
LDFLAGS = -m elf_i386 -T link.ld -nostdlib
ASFLAGS = -f elf32

OBJECTS = boot.o kernel.o

.PHONY: all clean run

all: kernel

kernel: $(OBJECTS)
	$(LD) $(LDFLAGS) -o kernel $(OBJECTS)

boot.o: boot.asm
	$(ASM) $(ASFLAGS) -o boot.o boot.asm

kernel.o: kernel.c
	$(CC) $(CFLAGS) -c kernel.c -o kernel.o

clean:
	rm -f *.o kernel

run: kernel
	qemu-system-i386 -kernel kernel

iso: kernel
	mkdir -p isodir/boot/grub
	cp kernel isodir/boot/kernel
	echo 'menuentry "KeprOS" { multiboot /boot/kernel }' > isodir/boot/grub/grub.cfg
	grub-mkrescue -o kepros.iso isodir

run-iso: iso
	qemu-system-i386 -cdrom kepros.iso
