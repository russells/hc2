#include "bsp.h"
#include "hc.h"
#include "ui.h"
#include "recorder.h"
#include "buttons.h"
#include "qpn_port.h"
#include "morse.h"
#include "serial.h"
#include <msp430.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


Q_DEFINE_THIS_MODULE("b");


#include "bsp-449STK2.h"


#define SB(_port,_bit) do { (_port) |=   (_bit);  } while(0)
#define CB(_port,_bit) do { (_port) &= (~(_bit)); } while(0)


static int16_t convert_adc_to_temperature(uint16_t adc);


/** If true, timer A is required and we want QP fast timer 1 running. */
static volatile uint8_t fast_timer_1 = 0;


/** If true, timer A is required and we want QP fast timer 2 running. */
static volatile uint8_t fast_timer_2 = 0;


void Q_onAssert(char const Q_ROM * const Q_ROM_VAR file, int line)
{
	BSP_stop_everything();
	serial_drain();
	serial_send("ASSERT: ");
	serial_send(file);
	serial_send_char(':');
	serial_send_int(line);
	serial_send("\r\n");
	serial_drain();
	morse_assert(file,line);
}


/** Initialise the basic timer but don't turn on its interrupt. */
static void
basic_timer1_init(void)
{
	/* Interrupt at 0.5Hz.  BTSSEL=0 (clock source is ACLK at 32768Hz),
	   BTDIV=1 (fCLK2 = ACLK/256), BTIPx=111 (interrupt at fCLK2/256).
	   (32768/256)/256 = 0.5.  */
	BTCTL = (BTSSEL & 0) |
		(BTHOLD & 0) |
		BTDIV |
		BTIP2 | BTIP1 | BTIP0;
}


static void timer_a_init(void)
{
	TACTL = (TASSEL1 & 0) | TASSEL0 | /* ACLK */
		(ID1 & 0) | (ID0 & 0) |	  /* /1 */
		(MC1 & 0) | (MC0 & 0) |	  /* Stop mode */
		(TAIE & 0);		  /* Interrupts off */
	TAR = 0;
	TACCR0 = 590;		/* With 32768Hz clock, about 18ms */

	TACCTL0 = CCIE;
}


/**
 * Ensure the fast timers are running.
 *
 * If either of both of the fast timers are required and Timer A is off, turn
 * on Timer A.  Otherwise, turn off Timer A.
 */
static void BSP_fast_timers(void)
{
	SERIALSTR("(");
	serial_send_int(fast_timer_1);
	SERIALSTR("*");
	serial_send_int(fast_timer_2);
	SERIALSTR(")");
	if (fast_timer_1 || fast_timer_2) {
		if (! (TACTL & MC0)) {
			SERIALSTR("+");
			if (fast_timer_1)
				SERIALSTR("1");
			if (fast_timer_2)
				SERIALSTR("2");
			/* Start TIMER_A */
			TAR = 0;
			SB(TACTL, MC0); /* Up mode */
		}
	} else {
		SERIALSTR("-");
		CB(TACTL, MC0);

		/* We are turning off the fast timer interrupt that scans the
		   buttons, so tell the buttons state machine about that. */
		postISR((QActive*)(&buttons), BUTTONS_SIGNAL, 0);
	}
}


/**
 * Turn the first fast timer on or off.
 *
 * We count up at turn on requests and down on turn off requests.  We turn off
 * the timer when the count gets to zero.  We don't decrement the counter below
 * zero.  All callers should have balanced on and off calls, but there is a
 * chance that a caller will not, and we may mask that behaviour with the
 * counter.
 *
 * @param onoff if true, the caller wants the first fast timer on.  Otherwise
 * the caller wants the first fast timer off.
 */
void BSP_fast_timer_1(uint8_t onoff)
{
	if (onoff) {
		fast_timer_1 ++;
	} else if (fast_timer_1) {
		fast_timer_1 --;
	}
	BSP_fast_timers();
}


/**
 * Turn the second fast timer on or off.
 *
 * @see BSP_fast_timer_1()
 */
void BSP_fast_timer_2(uint8_t onoff)
{
	if (onoff) {
		fast_timer_2 ++;
	} else if (fast_timer_2) {
		fast_timer_2 --;
	}
	BSP_fast_timers();
}


static void temperature_input_init(void)
{

	/* P6.1 is used to power the MCP9700A.  Ensure the pin is output as low
	   before we enable it, so we don't get a transient power supply on the
	   chip. */
	CB(P6OUT, BIT1);
	SB(P6DIR, BIT1);
	CB(P6SEL, BIT1);	/* IO function. */
	/* P6.0 is the analogue input.  Set it as a digital input for now, and
	   only change it to ADC input when required. */
	CB(P6OUT, BIT0);
	CB(P6DIR, BIT0);
	CB(P6SEL, BIT0);	/* IO function. */
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
	timer_a_init();

	temperature_input_init();

#ifdef LED
	P1DIR = BIT3;		/* LED output */

	BSP_led_on();
	__delay_cycles(2000000L); /* Wait at start so we can see resets. */
	BSP_led_off();
#endif

	/* Set up the switch inputs.  The 449STK2 board has external pullup
	   resistors on these pins. */
	P3DIR = 0;

	/* A pin for monitoring on time. */
	P2DIR |= BIT0;
}


void QF_onStartup(void)
{
	// Turn on the basic timer interrupt.
	IE2 |= BTIE;
}


void QF_onIdle(void)
{
	//SERIALSTR(",");
	BSP_led_off();
	P2OUT &= ~(BIT0);
#ifdef SERIAL
	/* If we're running the serial port (for debugging) don't stop SMCLK.
	   SMCLK keeps running while the CPU is running, and we don't shutdown
	   the CPU. */
	__enable_interrupt();
#else
	ENTER_LPM();
#endif
}


#ifdef LED
void BSP_led_on(void)
{
	P1OUT &= ~BIT3;
}


void BSP_led_off(void)
{
	P1OUT |= BIT3;
}
#endif


static void
__attribute__((__interrupt__(TIMERA0_VECTOR)))
isr_TIMERA0(void)
{
	uint16_t buttonmask = 0;
	static volatile uint8_t counter = 0;

	//serial_send_char(',');

	P2OUT |= BIT0;

	BSP_led_on();

	/* The buttons and QP-nano can't keep up with the rate we set with the
	   timer (18ms), so halve it. */
	counter ++;
	if (counter & 0x1) {
		if (fast_timer_1) {
			QF_tickXISR(1);
		}
		if (fast_timer_2) {
			QF_tickXISR(2);
		}
		if (BSP_button_1()) {
			buttonmask |= 1;
		}
		if (BSP_button_2()) {
			buttonmask |= 2;
		}
		if (BSP_button_3()) {
			buttonmask |= 4;
		}
		postISR((QActive*)(&buttons), BUTTONS_SIGNAL, (QParam)buttonmask);
	}
	EXIT_LPM();
}


static void
__attribute__((__interrupt__(BASICTIMER_VECTOR)))
isr_BASICTIMER(void)
{
	SERIALSTR("<B>");

	P2OUT |= BIT0;
	BSP_led_on();
	/* Only check button 1 if the fast timers are not on.  If they are on,
	   the buttons are checked inside that ISR. */
	if ( (! (fast_timer_1 || fast_timer_2)) &&  BSP_button_1()) {
		SERIALSTR("\\1");
		/* We send the button press signal here (rather than relying on
		   buttons.c to do it) because it's only when hc is told about
		   the button being down that it starts the fast timer. */
		postISR((QActive*)(&ui), BUTTON_1_PRESS_SIGNAL, 0);
		postISR((QActive*)(&buttons), BUTTONS_WAIT_SIGNAL, 0);
	}
	/* We have to call QF_tick() after the button events because some parts
	   of hc (hcTemperature() etc) ignore button events. */
	QF_tick();
	EXIT_LPM();
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
	// Turn off interrupts so nothing else happens.
	_BIC_SR(GIE);
}


void BSP_do_reset(void)
{
	// Ensure interrupts are off.
	_BIC_SR(GIE);
	// Enable the watchdog timer with ACLK as the source (since we know
	// that is turned on) and the shortest timeout.
	WDTCTL = WDTPW | WDTSSEL | WDTIS1 | WDTIS0;
	// Wait for the reset.
	while (1) {
		__delay_cycles(0xbedead);
	}
}


uint8_t BSP_button_1(void)
{
	return ! (P3IN & BIT4);
}


uint8_t BSP_button_2(void)
{
	return ! (P3IN & BIT5);
}


uint8_t BSP_button_3(void)
{
	return ! (P3IN & BIT6);
}


void BSP_start_temperature_reading(void)
{
	P6OUT |= BIT1;		/* Power up the MCP9700A */
	P6SEL |= BIT0;		/* A0 function on pin P6.0 */

	/* Ensure that ENC=0 before we start */
	ADC12CTL0 = 0;
	ADC12CTL0 =
		/* SHT1x = 0b0000. We use the extended sample mode. */
		(0xff00 & 0x0000)
		| (MSC & 0)
		| (REF2_5V & 0)	/* 1.5V reference */
		| (REFON)
		| (ADC12ON)
		| (ADC12OVIE & 0) /* No extra interupts */
		| (ADC12TOVIE & 0)
		| (ENC & 0)	/* ADC off */
		| (ADC12SC & 0);
	ADC12CTL1 =
		/* Select ADC12MEM10 */
		(0xf000 & 0xa000)
		/* Sample and hold source is ADC12SC */
		| (0x0c00 & 0x0000)
		/* Pulse sample mode */
		| (SHP & 0)
		/* Do not invert the SHP signal */
		| (ISSH & 0)
		/* ADC12DIVx == 000 */
		| (0x00e0 & 0x0000)
		/* clock is ACLK */
		| (ADC12SSEL1 & 0) | (ADC12SSEL0)
		/* single channel, single conversion */
		| (CONSEQ1 & 0) | (CONSEQ0 & 0);
	ADC12MCTL10 =
		/* VR+=Vref+, VR-=AVss */
		(SREF2 & 0) | (SREF1 & 0) | (SREF0)
		/* Input channel = 0000 = A0 */
		| (INCH3 & 0) | (INCH2 & 0) | (INCH1 & 0) | (INCH0 & 0);
	/* Reset all pending ADC12 interrupts. */
	ADC12IFG = 0;
	/* No more work here, as we need to wait for the reference to
	   stabilise.  The main QHsm will wiat an appropriate time, and call
	   BSP_get_temperature() to retrieve the reading. */
}


void BSP_get_temperature(void)
{
	/* Enable our interrupt. */
	ADC12IE = (1 << 10);
	/* Start the encoder.  Start the conversion by toggling ADC12SC. */
	_BIC_SR(GIE);
	ADC12CTL0 |= (ENC | ADC12SC);
	/* The temperature sensor requires 30us to stabilise. */
	__delay_cycles(30);
	ADC12CTL0 &= ~(ADC12SC);
	_BIS_SR(GIE);
	//Q_ASSERT(0); // yes
}


static void
__attribute__((__interrupt__(ADC12_VECTOR)))
isr_ADC12(void)
{
	uint16_t adc;
	int16_t temperature;
	static uint16_t previous_adc = 0x0002;
	static int16_t previous_temperature = 0;

	//Q_ASSERT(0); // yes
	P2OUT |= BIT0;

	adc = ADC12MEM10;
	serial_send_int(adc);
	/* The ADC value must move by at least four LSB before we regard it as
	   having changed.  This reduces jitter in the temperature readings. */
	if (abs(adc - previous_adc) >= 4) {
		temperature = convert_adc_to_temperature(adc);
		previous_temperature = temperature;
		previous_adc = adc;
	} else {
		temperature = previous_temperature;
		serial_send_int(previous_adc);
	}
	postISR((QActive*)(&recorder), TEMPERATURE_SIGNAL, temperature);
	ADC12IFG = 0;
	ADC12IE = 0;

	/* Turn off the ADC.  ENC first, so the other bits can be altered. */
	ADC12CTL0 &= ~(ENC);
	ADC12CTL0 = 0;

	CB(P6OUT, BIT1);	/* Power down the MCP9700A */
	SB(P6SEL, BIT0);	/* IO function on pin P6.0 */

	EXIT_LPM();
}


/**
 * Table entry that maps a range of ADC values to an encoded temperature value.
 */
struct TempConversion {
	uint16_t adcmin;	/** Minimum ADC value for this temperature. */
	uint16_t adcmax;	/** Maximum ADC value for this temperature. */
	int16_t ti;		/** Encoded temperature value. */
};

/* This .inc file contains definitions of the TempConversions array, the size
   of that array, and some extreme values that we use for direct comparison. */
#include "bsp-MCP9701A-temperature-scale.inc"


static int tccompare(const void *key, const void *entry)
{
	const struct TempConversion *keytc = key;
	const struct TempConversion *entrytc = entry;

	if (keytc->adcmin < entrytc->adcmin)
		return -1;
	else if (keytc->adcmin > entrytc->adcmax)
		return 1;
	else
		return 0;
}


/**
 * Convert the ADC reading to a temperature.
 *
 * @param adc the raw 12 bit ADC value
 *
 * @return a scaled temperature.  The temperature is int(T*2), which allows us
 * to represent temperatures to half a degree.
 *
 * T is in degrees C
 *
 * Vtemp = 0.00335(T) + 0.986 Volts
 * T = (Vtemp - 0.986) / 0.00335
 *
 * We first check for extreme values from the ADC (above upper bound or below
 * lower bound).  Then we do a binary search of the conversion array.
 */
static int16_t convert_adc_to_temperature(uint16_t adc)
{
	struct TempConversion keytc;
	struct TempConversion *matchtcp;

	if (adc <= MINADC) {
		return LOWTI;
	}
	if (adc >= MAXADC) {
		return HIGHTI;
	}
	/* We only use this one member of keytc, for comparisons inside
	   bsearch(). */
	keytc.adcmin = adc;
	matchtcp = (struct TempConversion *)
		bsearch(&keytc, tempconversions, NCONVERSIONS,
			sizeof(struct TempConversion), tccompare);
	if (! matchtcp) {
		Q_ASSERT(0);
	}
	return matchtcp->ti;
}


/** The name that's saved in flash to indicate the calibration.  We look for
    this exact string as part of the checks for a valid calibration value. */
static const char * const calibration_name = "CALIBRATION:";


/** The length of the name we save in flash.  This must match the length of
    calibration_name (ignoring its terminating null byte.) */
#define CALIBRATION_LEN 12


/** Base address of the calibration name, which is the base address of flash
    area.  This is the start of the "information flash" in the MSP430F449.  We
    use the first half (128 bytes) of this area. */
static char * const calibration_base = (char*) 0x1000;


/** The name that's saved in flash to indicate the time adjustment. */
static const char * const adjustment_name = "ADJUSTMENT:";

#define ADJUSTMENT_LEN 11

/** Base address of the adjustment name.  We allow for the length of the
    calibration name, plus three bytes for the sign, number, and ';'. */
static char * const adjustment_base = ((char *)(0x1000 + CALIBRATION_LEN + 3));


static int16_t get_saved_num(const char * const base,
			     const char *const name, uint8_t len)
{
	uint8_t negative;
	uint16_t num;

	SERIALSTR("get:");

	serial_send(base);

	if (strncmp(name, base, len)) {
		SERIALSTR("<A>\r\n");
		return 0;
	}
	if (base[len] == '-') {
		negative = 1;
	} else if (base[len] == '+') {
		negative = 0;
	} else {
		SERIALSTR("<B>0x");
		serial_send_hex_int(base[len]);
		SERIALSTR("\r\n");
		return 0;
	}
	if (base[len+1] < '0'
	    || base[len+1] > '9') {
		SERIALSTR("<C>\r\n");
		return 0;
	}
	if (base[len+2] != ';') {
		SERIALSTR("<D>\r\n");
		return 0;
	}
	if (negative) {
		num = '0' - base[len+1];
	} else {
		num = base[len+1] - '0';
	}
	SERIALSTR("\r\n");
	return num;

}


static int16_t calibration = 0;


static int16_t adjustment = 0;


/**
 * Get the calibration from nvram.
 */
int16_t BSP_get_calibration(void)
{
	calibration =  get_saved_num(calibration_base,
				     calibration_name, CALIBRATION_LEN);
	return calibration;
}


/**
 * Get the adjustment from nvram.
 */
int16_t BSP_get_adjustment(void)
{
	adjustment =  get_saved_num(adjustment_base,
				    adjustment_name, ADJUSTMENT_LEN);
	return adjustment;
}


static void save_nums(int16_t cal, int16_t adj)
{
	char *base;
	static char s[50];
	int slen;
	char c;
	uint8_t i;

	SERIALSTR("save:");

	Q_ASSERT( cal >= -9 );
	Q_ASSERT( cal <= 9 );
	Q_ASSERT( adj >= -9 );
	Q_ASSERT( adj <= 9 );

	slen = snprintf(s, 50, "%s%+d;%s%+d;",
			calibration_name, cal, adjustment_name, adj);
	Q_ASSERT( slen <= 49 );

	/* Do all the flash writing with interrupts off. */
	__disable_interrupt();

	/* Set up the flash write mechanism. */
	FCTL2 = FWKEY |
		(FSSEL1 & 0) | FSSEL0 | /* MCLK at 1048576Hz */
		0x02;		/* Divisor=3, 349525.3Hz */

	/* Clear the block. */
	FCTL3 = FWKEY | (LOCK & 0); /* LOCK=0 unlocks the flash segment */
	FCTL1 = FWKEY |
		(MERAS & 0) | ERASE;
	*calibration_base = '\0';
	FCTL3 = FWKEY | LOCK; /* LOCK=1 locks the flash segment */

	/* Write the string. */
	i = 0;
	base = (char *) calibration_base;
	do {
		c = s[i];
		if (c) {
			serial_send_char(c);
		} else {
			SERIALSTR("\\0");
		}
		FCTL3 = FWKEY | (LOCK * 0);
		FCTL1 = FWKEY | WRT;
		*base = c;
		FCTL3 = FWKEY | LOCK;
		base ++;
		i ++;
	} while (c);		/* Break _after_ writing the null byte. */

	SERIALSTR("\r\n");

	/* Interrupts back on. */
	__enable_interrupt();
}


void BSP_save_calibration(int16_t cal)
{
	calibration = cal;
	save_nums(cal, adjustment);
}


void BSP_save_adjustment(int16_t adj)
{
	adjustment = adj;
	save_nums(calibration, adj);
}
