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
	lcdm[index  ] |= *((uint8_t*)(&lb));
	lcdm[index+1] |= *(((uint8_t*)(&lb))+1);
#else
#error no lcdm code for big endian
#endif

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
