#include "lcd.h"
#include "qpn_port.h"
#include <msp430.h>
#include <stdint.h>
#include "serial.h"

Q_DEFINE_THIS_FILE;


/** A copy of the LCD register data specific to the four digit display at the
    top right of the LCD unit.  This data is merged into the LCD registers. */
static uint8_t lcdm_digits[20];

/** A copy of the LCD register data specific to the 14 segment characters.
    This data is merged into the LCD registers. */
static uint8_t lcdm_chars[20];

/** If set, the colon in the time display is on. */
static uint8_t colon = 0;

/** The number of timeout bars to display. */
static uint8_t timeouts = 0;


static void display_timeouts(void)
{
	switch (timeouts) {
	case 4:
		LCDM1 |= 0xf0;
		break;
	case 3:
		LCDM1 |= 0xe0;
		LCDM1 &= ~(0x10);
		break;
	case 2:
		LCDM1 |= 0xc0;
		LCDM1 &= ~(0x30);
		break;
	case 1:
		LCDM1 |= 0x80;
		LCDM1 &= ~(0x70);
		break;
	case 0:
		LCDM1 &= ~(0xf0);
		break;
	}
}


/** Turn on the colon if needed. */
static void display_colon(void)
{
	if (colon) {
		LCDM4 |= 0x80;
	} else {
		LCDM4 &= 0x7f;
	}
}


static void write_lcd_registers(void)
{
	volatile char *lcdm = LCDMEM;

	for (int i=0; i<20; i++) {
		lcdm[i] = (lcdm_digits[i] | lcdm_chars[i]);
	}
	display_colon();
	display_timeouts();
}


void lcd_init(void)
{
	uint8_t btctl;

	LCDM1  = 0x55; LCDM2  = 0x55; LCDM3  = 0x55; LCDM4  = 0x55;
	LCDM5  = 0x55; LCDM6  = 0x55; LCDM7  = 0x55; LCDM8  = 0x55;
	LCDM9  = 0x55; LCDM10 = 0x55; LCDM11 = 0x55; LCDM12 = 0x55;
	LCDM13 = 0x55; LCDM14 = 0x55; LCDM15 = 0x55; LCDM16 = 0x55;
	LCDM17 = 0x55; LCDM18 = 0x55; LCDM19 = 0x55; LCDM20 = 0x55;

	for (int i=0; i<20; i++) {
		lcdm_digits[i] = 0;
		lcdm_chars[i] = 0;
	}

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
	uint16_t lb = 0x0000;

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

	/*
	  Segments:

	       AAAAAAA
	     F H  J  K B
	     F  H J K  B
	     F   HJK   B
	       GGG MMM
	     E   QPN   C
	     E  Q P N  C
	     E Q  P  N C
	       DDDDDDD
        */

	/*
	  Mappings from LCD segments to MCU LCD register bits.  Later, we
	  extract this as two bytes, and worry about endianness there.

	  C and N are defined as status register bits in the MSP430 headers.
	 */
	static const uint16_t F = 0x4000;
	static const uint16_t E = 0x2000;
	static const uint16_t H = 0x0800;
	static const uint16_t G = 0x0400;
	static const uint16_t Q = 0x0200;
	static const uint16_t P = 0x0100;
	static const uint16_t A = 0x0080;
	static const uint16_t B = 0x0040;
#undef C
	static const uint16_t C = 0x0020;
	static const uint16_t D = 0x0010;
	static const uint16_t J = 0x0008;
	static const uint16_t K = 0x0004;
	static const uint16_t M = 0x0002;
#undef N
	static const uint16_t N = 0x0001;


	switch (ch & 0x7f) {		       /* ABCDEFGHJKMNPQ */
	case ' ': lb = 0;               break; /* -------------- */
	case '"': lb = B|J;             break;
	case '\'':lb = J;               break;
	case '+': lb = G|J|M|P;         break; /* ------G-J-M-P- */
	case '-': lb = G|M;             break; /* ------G---M--- */

	case '0': lb = A|B|C|D|E|F|K|Q; break; /* ABCDEF---K---Q */
	case '1': lb = B|C|K;           break; /* -BC------K---- */
	case '2': lb = A|B|D|E|G|M;     break; /* AB-DE-G---M--- */
	case '3': lb = A|B|C|D|M;       break; /* ABCD------M--- */
	case '4': lb = B|C|F|G|M;       break; /* -BC--FG---M--- */
	case '5': lb = A|D|F|G|N;       break; /* A--D-FG----N-- */
	case '6': lb = A|C|D|E|F|G|M;   break; /* A-CDEFG---M--- */
	case '7': lb = A|B|C;           break; /* ABC----------- */
	case '8': lb = A|B|C|D|E|F|G|M; break; /* ABCDEFG---M--- */
	case '9': lb = A|B|C|D|F|G|M;   break; /* ABCD-FG---M--- */

	case 'A': lb = A|B|C|E|F|G|M;   break; /* ABC-EFG---M--- */
	case 'B': lb = A|B|C|D|J|M|P;   break; /* ABCD----J-M-P- */
	case 'C': lb = A|D|E|F;         break; /* A--DEF-------- */
	case 'D': lb = A|B|C|D|J|P;     break; /* ABCD----J---P- */
	case 'E': lb = A|D|E|F|G;       break; /* A--DEFG------- */
	case 'F': lb = A|E|F|G;         break; /* A---EFG------- */
	case 'G': lb = A|C|D|E|F|M;     break; /* A-CDEF----M--- */
	case 'H': lb = B|C|E|F|G|M;     break; /* -BC-EFG---M--- */
	case 'I': lb = A|D|J|P;         break; /* A--D----J---P- */
	case 'J': lb = B|C|D|E;         break; /* -BCDE--------- */
	case 'K': lb = E|F|G|K|N;       break; /* ----EFG--K-N-- */
	case 'L': lb = D|E|F;           break; /* ---DEF-------- */
	case 'M': lb = B|C|E|F|H|K;     break; /* -BC-EF-H-K---- */
	case 'N': lb = B|C|E|F|H|N;     break; /* -BC-EF-H---N-- */
	case 'O': lb = A|B|C|D|E|F;     break; /* ABCDEF-------- */
	case 'P': lb = A|B|E|F|G|M;     break; /* AB--EFG---M--- */
	case 'Q': lb = A|B|C|D|E|F|N;   break; /* ABCDEF-----N-- */
	case 'R': lb = A|B|E|F|G|M|N;   break; /* AB--EFG---MN-- */
	case 'S': lb = A|C|D|F|G|M;     break; /* A-CD-FG---M--- */
	case 'T': lb = A|J|P;           break; /* A-------J---P- */
	case 'U': lb = B|C|D|E|F;       break; /* -BCDEF-------- */
	case 'V': lb = E|F|K|Q;         break; /* ----EF---K---Q */
	case 'W': lb = B|C|E|F|N|Q;     break; /* -BC-EF-----N-Q */
	case 'X': lb = H|K|N|Q;         break; /* -------H-K-N-Q */
	case 'Y': lb = B|F|G|M|P;       break; /* -B---FG---M-P- */
	case 'Z': lb = A|D|K|Q;         break; /* A--D------K--Q */

	case 'a': lb = D|E|G|H|P;       break;
	case 'b': lb = D|E|F|G|P;       break;
	case 'c': lb = D|E|G;           break;
	case 'd': lb = D|E|G|J|P;       break;
	case 'e': lb = D|E|G|Q;         break;
	case 'f': lb = A|J|M|P;         break;
	case 'g': lb = A|D|F|G|J|P;     break;
	case 'h': lb = E|F|G|P;         break;
	case 'i': lb = P;               break;
	case 'j': lb = E|J|Q;           break;
	case 'k': lb = J|K|N|P;         break;
	case 'l': lb = J|P;             break;
	case 'm': lb = C|E|G|M|P;       break;
	case 'n': lb = E|G|P;           break;
	case 'o': lb = C|D|E|G|M;       break;
	case 'p': lb = A|E|F|G|J;       break;
	case 'q': lb = A|F|G|J|P;       break;
	case 'r': lb = E|G;             break;
	case 's': lb = A|D|F|G|P;       break;
	case 't': lb = G|J|M|P;         break;
	case 'u': lb = D|E|P;           break;
	case 'v': lb = E|Q;             break;
	case 'w': lb = C|E|N|Q;         break;
	case 'x': lb = H|K|N|Q;         break;
	case 'y': lb = H|K|P;           break;
	case 'z': lb = D|G|Q;           break;
	default:
		Q_ASSERT(0);
		break;
	}
	//serial_send_hex_int(lb);
	//serial_send_char(' ');


	/* We only set the bits for the segments we've turned on, so we don't
	   accidentally turn off other segments. */
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	lcdm_chars[index-1] |= *((uint8_t*)(&lb));
	lcdm_chars[index  ] |= *(((uint8_t*)(&lb))+1);
#else
#error no lcdm code for big endian
#endif

	if (ch & 0x80) {
		/* There is no DP on the 7th character. */
		Q_ASSERT( pos != 6 );
		/* The DP is in the next lower LCD memory location, with
		   segments from the next character. */
		lcdm_chars[index-2] |= 0x10;
	}
}


void lcd_showstring(const char *s)
{
	uint8_t pos;

	//lcd_clear();

	for (int i=0; i<20; i++) {
		lcdm_chars[i] = 0;
	}

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
	write_lcd_registers();
	//SERIALSTR("\r\n");
}


static void lcd_showdigit(const char c, uint8_t pos)
{
	int index;
	uint8_t lb;

	/*
	  Mapping from LCD segments to MCU register bits for digits 8 and 9 on
	  the SBLCDA2.
	 */
	static const uint8_t A89 = 0x01;
	static const uint8_t B89 = 0x02;
	static const uint8_t C89 = 0x04;
	static const uint8_t D89 = 0x80;
	static const uint8_t E89 = 0x40;
	static const uint8_t F89 = 0x10;
	static const uint8_t G89 = 0x20;

	/*
	  Mapping from LCD segments to MCU register bits for digits 10 and 11
	  on the SBLCDA2.
	 */
	static const uint8_t A10 = 0x10;
	static const uint8_t B10 = 0x01;
	static const uint8_t C10 = 0x04;
	static const uint8_t D10 = 0x08;
	static const uint8_t E10 = 0x40;
	static const uint8_t F10 = 0x20;
	static const uint8_t G10 = 0x02;

	Q_ASSERT( pos < 4 );

	index = 5 - pos;

	if ( (0 == pos) || (1 == pos) ) {
		switch (c) {
		case '0': lb = A89|B89|C89|D89|E89|F89;     break;
		case '1': lb = B89|C89;                     break;
		case '2': lb = A89|B89|D89|E89|G89;         break;
		case '3': lb = A89|B89|C89|D89|G89;         break;
		case '4': lb = B89|C89|F89|G89;             break;
		case '5': lb = A89|C89|D89|F89|G89;         break;
		case '6': lb = A89|C89|D89|E89|F89|G89;     break;
		case '7': lb = A89|B89|C89;                 break;
		case '8': lb = A89|B89|C89|D89|E89|F89|G89; break;
		case '9': lb = A89|B89|C89|D89|F89|G89;     break;
		default:  lb = 0xff; Q_ASSERT( 0 );         break;
		}
	} else {
		switch (c) {
		case '0': lb = A10|B10|C10|D10|E10|F10;     break;
		case '1': lb = B10|C10;                     break;
		case '2': lb = A10|B10|D10|E10|G10;         break;
		case '3': lb = A10|B10|C10|D10|G10;         break;
		case '4': lb = B10|C10|F10|G10;             break;
		case '5': lb = A10|C10|D10|F10|G10;         break;
		case '6': lb = A10|C10|D10|E10|F10|G10;     break;
		case '7': lb = A10|B10|C10;                 break;
		case '8': lb = A10|B10|C10|D10|E10|F10|G10; break;
		case '9': lb = A10|B10|C10|D10|F10|G10;     break;
		default:  lb = 0xff; Q_ASSERT( 0 );         break;
		}
	}

	/* We have to OR in the bits we want set as the registers for these
	   digits have bits for more than one digit. */
	lcdm_digits[index] |= lb;
}


void lcd_showdigits(const char *ds)
{
	uint8_t i;

	for (i=0; i<20; i++) {
		lcdm_digits[i] = 0;
	}

	for (i=0; i<4; i++) {
		lcd_showdigit(ds[i], i);
	}

	write_lcd_registers();
}


void lcd_colon(uint8_t onoff)
{
	colon = onoff;
	display_colon();
}


void lcd_timeouts(uint8_t t)
{
	/* For the development hardware (the Olimex MSP430-449STk2) we are
	   using the available segments which are the "units" segments.

	   @todo: when the real hardware is available, change to using the
	   middle four progress bar segments.  This may not require any code
	   changes, depending on the final segment connections.
	*/

	Q_ASSERT( t < 5 );
	timeouts = t;
	write_lcd_registers();
}
