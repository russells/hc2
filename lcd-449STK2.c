#include "lcd.h"
#include "qpn_port.h"
#include <msp430.h>
#include <stdint.h>

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
 * Given a string to show, figure out what LCD segments to activate.
 */
void lcd_show(const char *s)
{
	uint8_t c = (uint8_t)(s[0]);

	LCDM1  = c; LCDM2  = c; LCDM3  = c; LCDM4  = c;
	LCDM5  = c; LCDM6  = c; LCDM7  = c; LCDM8  = c;
	LCDM9  = c; LCDM10 = c; LCDM11 = c; LCDM12 = c;
	LCDM13 = c; LCDM14 = c; LCDM15 = c; LCDM16 = c;
	LCDM17 = c; LCDM18 = c; LCDM19 = c; LCDM20 = c;
}


void lcd_clear(void)
{
	lcd_show("");
}


void lcd_showchar(char ch, uint8_t pos)
{
	uint8_t index;
	volatile char *lcdm = LCDMEM - 1; /* LCD register names are 1-based. */

	switch (pos) {
	case 0:
		index = 19;
		break;
	case 1:
		index = 17;
		break;
	case 2:
		index = 15;
		break;
	case 3:
		index = 13;
		break;
	case 4:
		index = 11;
		break;
	case 5:
		index = 9;
		break;
	case 6:
		index = 7;
		break;
	default:
		index = 77;
		Q_ASSERT(0);
		break;
	}

	switch (ch) {
	case '0':
		lcdm[index+1] = 0x62;
		lcdm[index  ] = 0xf4;
		break;
	case '1':
		lcdm[index  ] = 0x60;
		break;
	case '2':
		lcdm[index+1] = 0x24;
		lcdm[index  ] = 0xd2;
		break;
	case '3':
		lcdm[index+1] = 0x04;
		lcdm[index  ] = 0xf2;
		break;
	case '4':
		lcdm[index+1] = 0x44;
		lcdm[index  ] = 0x62;
		break;
	case '5':
		lcdm[index+1] = 0x44;
		lcdm[index  ] = 0x91;
		break;
	case '6':
		lcdm[index+1] = 0x64;
		lcdm[index  ] = 0xb2;
		break;
	case '7':
		lcdm[index  ] = 0xe0;
		break;
	case '8':
		lcdm[index+1] = 0x64;
		lcdm[index  ] = 0xf2;
		break;
	case '9':
		lcdm[index+1] = 0x44;
		lcdm[index  ] = 0xf2;
		break;
	case 'H':
		lcdm[index+1] = 0x64;
		lcdm[index  ] = 0x62;
		break;
	case 'O':
		lcdm[index+1] = 0x60;
		lcdm[index  ] = 0xf0;
		break;
	case 'T':
		lcdm[index+1] = 0x01;
		lcdm[index  ] = 0x88;
		break;
	case 'S':
		lcdm[index+1] = 0x44;
		lcdm[index  ] = 0xb2;
		break;
	case 'C':
		lcdm[index+1] = 0x60;
		lcdm[index  ] = 0x90;
		break;
	case 'L':
		lcdm[index+1] = 0x60;
		lcdm[index  ] = 0x10;
		break;
	case 'D':
		lcdm[index+1] = 0x01;
		lcdm[index  ] = 0xf8;
		break;
	case ' ':
		lcdm[index+1] = 0x00;
		lcdm[index  ] = 0x00;
		break;
	default:
		Q_ASSERT(0);
		break;
	}
}


void lcd_showstring(const char *s, uint8_t startpos)
{
	while (*s) {
		lcd_showchar(*s, startpos);
		s++;
		startpos++;
	}
}
