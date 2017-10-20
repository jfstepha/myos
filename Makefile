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
	(cd image; make clean)

dep:
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make
	(for i in init/*.c;do echo -n "init/";$(CPP) -M $$i;done) >> tmp_make
	cp tmp_make Makefile
### Dependencies:	
init/main.o: init/main.c /usr/include/stdc-predef.h include/time.h \
 include/keyboard_map.h include/stdarg.h include/asm/io.h \
 include/linux/tty.h
