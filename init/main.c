
/*
* Copyright (C) 2014  Arjun Sreedharan
* License: GPL version 2 or higher http://www.gnu.org/licenses/gpl.html
* other parts of this code plagerized from linux kernel starting with kerne rev 0.01
*/
#define __LIBRARY__
#include <time.h>
#include "keyboard_map.h"
#include <stdarg.h>

#include <asm/io.h>
#include <linux/tty.h>

/************************************************************************************
 * this is all stuff from kernel 101
 * it should be deleted when we get linux code in here
 * **********************************************************************************/
/* there are 25 lines each of 80 columns; each element takes 2 bytes */

#define LINES 25
#define COLUMNS_IN_LINE 80
#define BYTES_FOR_EACH_ELEMENT 2
#define SCREENSIZE BYTES_FOR_EACH_ELEMENT * COLUMNS_IN_LINE * LINES

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define IDT_SIZE 256
#define INTERRUPT_GATE 0x8e
#define KERNEL_CODE_SEGMENT_OFFSET 0x08

#define ENTER_KEY_CODE 0x1C
#define MAX_INT_PRINT 1000000000

extern unsigned char keyboard_map[128];
extern void keyboard_handler(void);
extern char read_port(unsigned short port);
extern void write_port(unsigned short port, unsigned char data);
extern void load_idt(unsigned long *idt_ptr);

/* current cursor location */
unsigned int current_loc = 0;
/* video memory begins at address 0xb8000 */
char *vidptr = (char*)0xb8000;

struct IDT_entry {
	unsigned short int offset_lowerbits;
	unsigned short int selector;
	unsigned char zero;
	unsigned char type_attr;
	unsigned short int offset_higherbits;
};

struct IDT_entry IDT[IDT_SIZE];


void idt_init(void)
{
	unsigned long keyboard_address;
	unsigned long idt_address;
	unsigned long idt_ptr[2];

	/* populate IDT entry of keyboard's interrupt */
	keyboard_address = (unsigned long)keyboard_handler;
	IDT[0x21].offset_lowerbits = keyboard_address & 0xffff;
	IDT[0x21].selector = KERNEL_CODE_SEGMENT_OFFSET;
	IDT[0x21].zero = 0;
	IDT[0x21].type_attr = INTERRUPT_GATE;
	IDT[0x21].offset_higherbits = (keyboard_address & 0xffff0000) >> 16;

	/*     Ports
	*	 PIC1	PIC2
	*Command 0x20	0xA0
	*Data	 0x21	0xA1
	*/

	/* ICW1 - begin initialization */
	write_port(0x20 , 0x11);
	write_port(0xA0 , 0x11);

	/* ICW2 - remap offset address of IDT */
	/*
	* In x86 protected mode, we have to remap the PICs beyond 0x20 because
	* Intel have designated the first 32 interrupts as "reserved" for cpu exceptions
	*/
	write_port(0x21 , 0x20);
	write_port(0xA1 , 0x28);

	/* ICW3 - setup cascading */
	write_port(0x21 , 0x00);
	write_port(0xA1 , 0x00);

	/* ICW4 - environment info */
	write_port(0x21 , 0x01);
	write_port(0xA1 , 0x01);
	/* Initialization finished */

	/* mask interrupts */
	write_port(0x21 , 0xff);
	write_port(0xA1 , 0xff);

	/* fill the IDT descriptor */
	idt_address = (unsigned long)IDT ;
	idt_ptr[0] = (sizeof (struct IDT_entry) * IDT_SIZE) + ((idt_address & 0xffff) << 16);
	idt_ptr[1] = idt_address >> 16 ;

	load_idt(idt_ptr);
}

void kb_init(void)
{
	/* 0xFD is 11111101 - enables only IRQ1 (keyboard)*/
	write_port(0x21 , 0xFD);
}
void kprint_newline(void)
{
	unsigned int line_size = BYTES_FOR_EACH_ELEMENT * COLUMNS_IN_LINE;
	current_loc = current_loc + (line_size - current_loc % (line_size));
}

void kprint(const char *str)
{
	unsigned int i = 0;
	while (str[i] != '\0') {
		vidptr[current_loc++] = str[i++];
		vidptr[current_loc++] = 0x07;
	}
}

const char *digits = "0123456789";

void kprint_int(int a) {
    int tmp;
    int i;
    int digit;
    int j=0;
    char c;
    char *tmp_str = "...............";
    int leading=1;
    // kprint_newline();
    // kprint("printing number:");
    if( a == 0) {
        kprint("0");
        return;
    }
    if (a < 0) {
        kprint("-");
        tmp = -a;
    } else {
        tmp = a;
    }
    if (tmp > MAX_INT_PRINT) {
    	kprint(">MAX");
    } else {
    	i = MAX_INT_PRINT;
    	while( i > 1 ) {
    		i /= 10;
    		digit = tmp / i;
    		if ( digit != 0) leading = 0;
    		if ( leading == 0 || digit != 0 ) {
    			tmp_str[j] = digits[digit];
         		j++;
    		}
    		tmp  = tmp % i;
    	} while (i > 1);
    	tmp_str[j] = 0;
    	kprint(tmp_str);

    }


}

void clear_screen(void)
{
	unsigned int i = 0;
	while (i < SCREENSIZE) {
		vidptr[i++] = ' ';
		vidptr[i++] = 0x07;
	}
}

void keyboard_handler_main(void)
{
	unsigned char status;
	char keycode;

	/* write EOI */
	write_port(0x20, 0x20);

	status = read_port(KEYBOARD_STATUS_PORT);
	/* Lowest bit of status will be set if buffer is not empty */
	if (status & 0x01) {
		keycode = read_port(KEYBOARD_DATA_PORT);
		if(keycode < 0)
			return;

		if(keycode == ENTER_KEY_CODE) {
			kprint_newline();
			return;
		}

		vidptr[current_loc++] = keyboard_map[(unsigned char) keycode];
		vidptr[current_loc++] = 0x07;
	}
}
/************************************************************************************
 * end kernel 101 stuff
 */

#define CMOS_READ(addr) ({ \
	outb_p(0x80|addr,0x70); \
	inb_p(0x71); \
})

#define BCD_TO_BIN(val) ((val)=((val)&15) + ((val)>>4)*10)
static void time_init(void)
{
	struct tm time;
	do {
		time.tm_sec = CMOS_READ(0);
		time.tm_min = CMOS_READ(2);
		time.tm_hour = CMOS_READ(4);
		time.tm_mday = CMOS_READ(7);
		time.tm_mon = CMOS_READ(8)-1;
		time.tm_year = CMOS_READ(9);
	} while(time.tm_sec != CMOS_READ(0));
	BCD_TO_BIN(time.tm_sec);
	BCD_TO_BIN(time.tm_min);
	BCD_TO_BIN(time.tm_hour);
	BCD_TO_BIN(time.tm_mday);
	BCD_TO_BIN(time.tm_mon);
	BCD_TO_BIN(time.tm_year);

	kprint("* time: ");
	kprint_int(time.tm_hour);
	kprint(":");
	kprint_int(time.tm_min);
	kprint(" ");
	kprint_int(time.tm_mon);
	kprint("/");
	kprint_int(time.tm_mday);
	kprint("/");
	kprint_int(time.tm_year);
	kprint_newline();

}


void kmain(void)
{
    // 0x100642 in current simics build
	clear_screen();
	kprint( "*********** my first kernel ***********");
	kprint_newline();
	kprint_newline();
	kprint_newline();

	kprint("* time_init:"); kprint_newline();
	time_init();

	kprint("* idt_init:"); kprint_newline();
	idt_init();
	kprint("* kb_init:"); kprint_newline();
	kb_init();
	kprint("* tty_init:"); kprint_newline();
	tty_init();


	while(1);
}

// i don't think printf will work until tty is initialized
static int printf(const char *fmt, ...) {
    va_list args;
    int i;

    va_start(args, fmt);


}
