#include "lcd.h"
#include <msp430.h>
#include <stdint.h>


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


void lcd_showdigit(uint8_t digit)
{
	volatile char *lcdm = LCDMEM - 1; /* LCD register names are 1-based. */

	lcd_show("");
	switch (digit) {
	case 0:
		lcdm[20] = 0x60;
		lcdm[19] = 0xf0;
		break;
	case 1:
		lcdm[19] = 0x60;
		break;
	case 2:
		lcdm[20] = 0x24;
		lcdm[19] = 0xd2;
		break;
	case 3:
		lcdm[20] = 0x04;
		lcdm[19] = 0xf2;
		break;
	case 4:
		lcdm[20] = 0x44;
		lcdm[19] = 0x62;
		break;
	case 5:
		lcdm[20] = 0x44;
		lcdm[19] = 0xb2;
		break;
	case 6:
		lcdm[20] = 0x64;
		lcdm[19] = 0xb2;
		break;
	case 7:
		lcdm[19] = 0xe0;
		break;
	case 8:
		lcdm[20] = 0x64;
		lcdm[19] = 0xf2;
		break;
	case 9:
		lcdm[20] = 0x44;
		lcdm[19] = 0xf2;
		break;
	}
}
