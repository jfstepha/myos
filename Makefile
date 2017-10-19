CC = gcc -Iinclude
CFLAGS = -m32
all: kernel-exe

kernel-exe: kasm.o main.o kernel/kernel.o
	ld -m elf_i386 -T link.ld -o kernel-exe kasm.o main.o kernel/kernel.o

main.o: init/main.c
	$(CC) -m32 -c init/main.c -o main.o
	
kernel/kernel.o:
	(cd kernel; make)	

kasm.o: kernel.asm
	nasm -f elf32 kernel.asm -o kasm.o

clean:
	rm *.o
	rm kernel/*.o
	rm kernel-exe
