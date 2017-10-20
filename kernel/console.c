/*
 * console.c
 *
 *  Created on: Oct 18, 2017
 *      Author: jfstepha
 */

#define SCREEN_START 0xb8000
#define SCREEN_END   0xc0000
#define LINES 25
#define COLUMNS 80
#define NPAR 16

extern void keyboard_interrupt(void);

static unsigned long origin = SCREEN_START;
static unsigned long scr_end = SCREEN_START + LINES * COLUMNS * 2;
static unsigned long pos;
static unsigned long x,y;
static unsigned long top=0, bottom = LINES;
static unsigned long lines = LINES, columns = COLUMNS;
static unsigned long state = 0;
static unsigned long npar, par[NPAR];
static unsigned long ques = 0;
static unsigned char attr = 0x07;

static inline void gotoxy(unsigned int new_x, unsigned int new_y) {
    if( new_x >= columns || new_y>=lines)
        return;
    x = new_x;
    y = new_y;
    pos = origin + ( ( y * columns + x) << 1);

}

void con_init(void){
    register unsigned char a;
    gotoxy(*(unsigned char *)(0x90000+510),*(unsigned char *)(0x90000+511));

}
