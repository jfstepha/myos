
all: myos.iso

myos.iso: grub.cfg ../kernel-exe
	cp ../kernel-exe isodir/boot/myos.bin
	cp grub.cfg isodir/boot/grub
	grub-mkrescue -o myos.iso isodir

clean:
	rm -f *.iso
	rm -f isodir/boot/myos.bin
