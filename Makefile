# Makefile
CC = gcc
CFLAGS = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs -Wall -Wextra -c
AS = nasm
ASFLAGS = -f elf32
LD = ld
LDFLAGS = -m elf_i386 -T link.ld

all: kernel

kernel: boot.o kernel.o
	$(LD) $(LDFLAGS) -o kernel boot.o kernel.o

boot.o: boot.s
	$(AS) $(ASFLAGS) boot.s -o boot.o

kernel.o: kernel.c
	$(CC) $(CFLAGS) kernel.c -o kernel.o

run: kernel
	qemu-system-i386 -kernel kernel -netdev user,id=net0 -device e1000,netdev=net0

clean:
	rm -f *.o kernel