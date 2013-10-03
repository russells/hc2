#include "bsp.h"
#include "qpn_port.h"
#include "morse.h"
#include <msp430.h>


void Q_onAssert(char const Q_ROM * const Q_ROM_VAR file, int line)
{
	morse_assert(file,line);
}


static void
basic_timer1_init(void)
{
	// Initialise the basic timer but don't turn on its interrupt.
	BTCTL = (BTSSEL & 0) |
		(BTHOLD & 0) |
		BTDIV |
		// TODO: work out the interrupt rate.
		// interrupt at fCLK2/16
		(BTIP2 & 0) | BTIP1 | BTIP0;
}


void BSP_init(void)
{
	WDTCTL = WDTPW | WDTHOLD; /* Stop the watchdog */

	FLL_CTL0 &= ~XTS_FLL;	/* XT1 as low-frequency */
	_BIC_SR(OSCOFF);	/* turn on XT1 oscillator */

	do {			/* wait in loop until crystal is stable */
		IFG1 &= ~OFIFG;
	} while (IFG1 & OFIFG);

	// Interrupts on.
	_BIS_SR(GIE);

	basic_timer1_init();

	P1DIR = BIT3;		/* LED output */
}


void QF_onStartup(void)
{
	// Turn on the basic timer interrupt.
	IE2 |= BTIE;
}


void QF_onIdle(void)
{
	// TODO: low power mode.
	_BIS_SR(GIE);		/* Enable interrupts */
}


void BSP_led_on(void)
{
	P1OUT &= ~BIT3;
}


void BSP_led_off(void)
{
	P1OUT |= BIT3;
}


static void
__attribute__((__interrupt__(BASICTIMER_VECTOR)))
isr_BASICTIMER(void)
{
	QF_tick();
}


void BSP_enable_morse_line(void)
{
	P1DIR = BIT3;
}


void BSP_morse_signal(uint8_t onoff)
{
	if (onoff) {
		P1OUT &= ~BIT3;
	} else {
		P1OUT |= BIT3;
	}
}


void BSP_stop_everything(void)
{

}


void BSP_do_reset(void)
{

}

