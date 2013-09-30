
#include  <msp430x44x.h>
#include  <inttypes.h>

#define          LED_OFF            P1OUT |= BIT3
#define          LED_ON             P1OUT &= ~BIT3


static void
basic_timer1_init(void)
{
	BTCTL = (BTSSEL & 0) |
		(BTHOLD & 0) |
		BTDIV |
		// interrupt at fCLK2/16
		(BTIP2 & 0) | BTIP1 | BTIP0;
	IE2 |= BTIE;
}


int main(void)
{
	WDTCTL = WDTPW | WDTHOLD; /* Stop the watchdog */

	FLL_CTL0 &= ~XTS_FLL;	/* XT1 as low-frequency */
	_BIC_SR(OSCOFF);	/* turn on XT1 oscillator */

	do {			/* wait in loop until crystal is stable */
		IFG1 &= ~OFIFG;
	} while (IFG1 & OFIFG);

	basic_timer1_init();

	P1DIR = BIT3;		/* LED output */

	LED_ON;
	__delay_cycles(1000000L);
	LED_OFF;

	_BIS_SR(GIE);		/* Enable interrupts */

	while (1) {
		__delay_cycles(500000L); /* 0.5 second wait, assuming a 1MHz
					    clock. */
		//LED_ON;
		__delay_cycles(500000L);
		//LED_OFF;
	}
	return 0;
}


static void
__attribute__((__interrupt__(BASICTIMER_VECTOR)))
isr_BASICTIMER(void)
{
	static uint8_t onoff = 0;
	if (onoff) {
		LED_ON;
		onoff = 0;
	} else {
		LED_OFF;
		onoff = 1;
	}
}
