
#include  <msp430x44x.h>

#define          LED_OFF            P1OUT |= BIT3
#define          LED_ON             P1OUT &= ~BIT3

void main(void)
{
	WDTCTL = WDTPW | WDTHOLD; /* Stop the watchdog */

	FLL_CTL0 &= ~XTS_FLL;	/* XT1 as low-frequency */
	_BIC_SR(OSCOFF);	/* turn on XT1 oscillator */

	do {			/* wait in loop until crystal is stable */
		IFG1 &= ~OFIFG;
	} while (IFG1 & OFIFG);

	P1DIR = BIT3;		/* LED output */

	while (1) {
		__delay_cycles(500000L); /* 0.5 second wait, assuming a 1MHz
					    clock. */
		LED_ON;
		__delay_cycles(500000L);
		LED_OFF;
	}
}
