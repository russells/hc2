#include "lcd.h"
#include "qpn_port.h"
#include <msp430.h>
#include <stdint.h>
#include "serial.h"

Q_DEFINE_THIS_FILE;


void lcd_init(void)
{
	uint8_t btctl;

	LCDM1  = 0x55; LCDM2  = 0x55; LCDM3  = 0x55; LCDM4  = 0x55;
	LCDM5  = 0x55; LCDM6  = 0x55; LCDM7  = 0x55; LCDM8  = 0x55;
	LCDM9  = 0x55; LCDM10 = 0x55; LCDM11 = 0x55; LCDM12 = 0x55;
	LCDM13 = 0x55; LCDM14 = 0x55; LCDM15 = 0x55; LCDM16 = 0x55;
	LCDM17 = 0x55; LCDM18 = 0x55; LCDM19 = 0x55; LCDM20 = 0x55;

	/* Assume the timer is already set up, and only change the bits for the
	   LCD. */
	btctl = BTCTL;
	/* Frame rate = fACLK/BTFRFQxDIV/(2*mux).  BTFRFQxDIV=128, mux=4, so
	   frame rate = 32Hz */
	btctl |= BTFRFQ1 | (BTFRFQ0 & 0);
	BTCTL = btctl;

	LCDCTL = LCDP2 | LCDP1 | LCDP0 /* S0-S39 */
		| LCDMX1 | LCDMX0      /* 4-mux */
		| LCDSON | LCDON;
	P5SEL = 0xfc;		/* This works, but why not P4SEL too? */
}


/**
 * Set all the segment registers to the same value.
 */
void lcd_setsegments(uint8_t c)
{
	LCDM1  = c; LCDM2  = c; LCDM3  = c; LCDM4  = c;
	LCDM5  = c; LCDM6  = c; LCDM7  = c; LCDM8  = c;
	LCDM9  = c; LCDM10 = c; LCDM11 = c; LCDM12 = c;
	LCDM13 = c; LCDM14 = c; LCDM15 = c; LCDM16 = c;
	LCDM17 = c; LCDM18 = c; LCDM19 = c; LCDM20 = c;
}


void lcd_clear(void)
{
	lcd_setsegments(0);
}


void lcd_showchar(char ch, uint8_t pos)
{
	uint8_t index;
	volatile char *lcdm = LCDMEM - 1; /* LCD register names are 1-based. */
	uint8_t lcdm1 = 0xff;
	uint8_t lcdm0 = 0xff;

	switch (pos) {
	case 0: index = 19; break;
	case 1: index = 17; break;
	case 2: index = 15; break;
	case 3: index = 13; break;
	case 4: index = 11; break;
	case 5: index = 9; break;
	case 6: index = 7; break;
	default:
		index = 77;
		Q_ASSERT(0);
		break;
	}

	switch (ch & 0x7f) {
	case '0': lcdm1 = 0x62; lcdm0 = 0xf4; break;
	case '1': lcdm1 = 0x00; lcdm0 = 0x60; break;
	case '2': lcdm1 = 0x24; lcdm0 = 0xd2; break;
	case '3': lcdm1 = 0x04; lcdm0 = 0xf2; break;
	case '4': lcdm1 = 0x44; lcdm0 = 0x62; break;
	case '5': lcdm1 = 0x44; lcdm0 = 0x91; break;
	case '6': lcdm1 = 0x64; lcdm0 = 0xb2; break;
	case '7': lcdm1 = 0x00; lcdm0 = 0xe0; break;
	case '8': lcdm1 = 0x64; lcdm0 = 0xf2; break;
	case '9': lcdm1 = 0x44; lcdm0 = 0xf2; break;
	case 'H': lcdm1 = 0x64; lcdm0 = 0x62; break;
	case 'O': lcdm1 = 0x60; lcdm0 = 0xf0; break;
	case 'T': lcdm1 = 0x01; lcdm0 = 0x88; break;
	case 'S': lcdm1 = 0x44; lcdm0 = 0xb2; break;
	case 'C': lcdm1 = 0x60; lcdm0 = 0x90; break;
	case 'L': lcdm1 = 0x60; lcdm0 = 0x10; break;
	case 'D': lcdm1 = 0x01; lcdm0 = 0xf8; break;
	case ' ': lcdm1 = 0x00; lcdm0 = 0x00; break;
	case 'N': lcdm1 = 0x68; lcdm0 = 0x61; break;
	case 'R': lcdm1 = 0x64; lcdm0 = 0xc3; break;
	case 'M': lcdm1 = 0x68; lcdm0 = 0x64; break;
	case 'A': lcdm1 = 0x64; lcdm0 = 0xe2; break;
	case '-': lcdm1 = 0x04; lcdm0 = 0x02; break;
	case '+': lcdm1 = 0x05; lcdm0 = 0x0a; break;
	default:
		Q_ASSERT(0);
		break;
	}
	//serial_send_hex_int(lcdm1);
	//serial_send_char(',');
	//serial_send_hex_int(lcdm0);
	//serial_send_char(' ');
	/* We only set the bits for the segments we've turned on, so we don't
	   accidentally turn off other segments. */
	lcdm[index+1] |= lcdm1;
	lcdm[index  ] |= lcdm0;
	if (ch & 0x80) {
		/* The DP is in the next lower LCD memory location, with
		   segments from the next character. */
		lcdm[index-1] |= 0x10;
	}
}


void lcd_showstring(const char *s)
{
	uint8_t pos;

	lcd_clear();
	//SERIALSTR("S: \"");
	//serial_send(s);
	//SERIALSTR("\"\r\n");
	//SERIALSTR("-- ");
	pos = 0;
	while (*s) {
		lcd_showchar(*s, pos);
		s++;
		pos++;
	}
	//SERIALSTR("\r\n");
}
