#include "bsp.h"
#include "hc.h"
#include "qpn_port.h"
#include "morse.h"
#include "serial.h"
#include <msp430.h>
#include <stdlib.h>
#include <string.h>


Q_DEFINE_THIS_FILE;


#include "bsp-449STK2.h"


static int16_t convert_adc_to_temperature(uint16_t adc);


/**
 * The number of ticks we do in slow timer mode between returns to QP-nano.
 */
#define SLOW_TIMER_TICKS 4


/**
 * Set when we have the fast timer tick running.
 *
 * When set, we return to the QP-nano loop after every tick.  When not set, we
 * only return every SLOW_TIMER_TICKS ticks, to reduct power consumption.
 */
static volatile uint8_t fast_timer = 0;


/**
 * Count slow timer ticks, so we know when to return to QP-nano.
 */
static volatile uint8_t slow_timer_ticks = 0;


/**
 * Set to tell QF_onIdle() that we need to do QP-nano processing now.
 */
static volatile uint8_t timer_tick_done = 0;


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


static void basic_timer1_rate(uint8_t rate)
{
	uint8_t btctl;

	BTCTL |= BTHOLD;	/* Stop the timer. */
	BTCNT1 = 0;		/* Reset the counters. */
	BTCNT2 = 0;
	btctl = BTCTL;
	/* Reset the BTIPx bits.*/
	btctl &= ~(BTIP2 | BTIP2 | BTIP0);
	btctl |= rate;		/* Now set the desired BTIPx bits. */
	BTCTL = btctl;
	BTCTL &= ~(BTHOLD);	/* Restart the timer. */
}


void BSP_fast_timer(void)
{
	/* BTIPx=001, fCLK2/2 = 32Hz, period = 31.25ms.  The ADC reference
	 wants 17ms to stabilise, so the next shorter period (15.625ms) is too
	 short. */
	basic_timer1_rate(0b001);
	fast_timer = 77;
}


/**
 * Change the timer to give us long intervals.
 *
 * @param reset if true, start counting from the longest interval.
 */
void BSP_slow_timer(uint8_t reset)
{
	// BTIPx=111, fCLK2/256 = 0.5HZ, period = 2s
	basic_timer1_rate(0b111);
	fast_timer = 0;
	if (reset) {
		slow_timer_ticks = 0;
	}
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

#ifdef LED
	P1DIR = BIT3;		/* LED output */

	BSP_led_on();
	__delay_cycles(2000000L); /* Wait at start so we can see resets. */
	BSP_led_off();
#endif

	/* Set up the switch inputs.  The 449STK2 board has external pullup
	   resistors on these pins. */
	P3DIR = 0;
}


void QF_onStartup(void)
{
	// Turn on the basic timer interrupt.
	IE2 |= BTIE;
}


void QF_onIdle(void)
{
 idle:
	BSP_led_off();
	ENTER_LPM();
	if (! timer_tick_done) {
		goto idle;
	}
	/* If timer_tick_done was true, we reset it for next time, and return
	   to the QP-nano loop. */
	timer_tick_done = 0;
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
__attribute__((__interrupt__(BASICTIMER_VECTOR)))
isr_BASICTIMER(void)
{
	BSP_led_on();
	if (fast_timer) {
		SERIALSTR("<F>");
		QF_tick();
		timer_tick_done = 1;
	} else {
		if (! slow_timer_ticks) {
			SERIALSTR("<S:Q>");
			QF_tick();
			slow_timer_ticks = SLOW_TIMER_TICKS;
			timer_tick_done = 1;
		} else {
			if (BSP_cal_switch()) {
				SERIALSTR("<S:Sw>");
				QF_tick();
				timer_tick_done = 1;
			} else {
				SERIALSTR("<S>");
			}
			slow_timer_ticks --;
		}
	}
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


uint8_t BSP_cal_switch(void)
{
	return ! (P3IN & BIT4);
}


uint8_t BSP_up_switch(void)
{
	return ! (P3IN & BIT5);
}


uint8_t BSP_down_switch(void)
{
	return ! (P3IN & BIT6);
}


void BSP_start_temperature_reading(void)
{
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
		/* Input channel = 1010 = temperature sensor */
		| (INCH3) | (INCH2 & 0) | (INCH1) | (INCH0 & 0);
	/* Reset all pending ADC12 interrupts. */
	ADC12IFG = 0;
	/* No more work here, as we need to wait for the reference to
	   stabilise.  The main QHsm will wiat an appropriate time, and call
	   BSP_get_temperature() to retrieve the reading. */
}


void BSP_get_temperature(void)
{
	/* Wait for the voltage reference to stabilise. */
	__delay_cycles(17000);	/* FIXME do this with an event? */
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

	adc = ADC12MEM10;
	SERIALSTR("A: ");
	serial_send_int(adc);
	/* The ADC value must move by at least two LSB before we regard it as
	   having changed.  This reduces jitter in the temperature readings. */
	if (abs(adc - previous_adc) >= 2) {
		temperature = convert_adc_to_temperature(adc);
		previous_temperature = temperature;
		previous_adc = adc;
	} else {
		temperature = previous_temperature;
		SERIALSTR("(");
		serial_send_int(previous_adc);
		SERIALSTR(")");
	}
	serial_send_char(':');
	serial_send_int(temperature);
	SERIALSTR("\r\n");
	QActive_postISR((QActive*)(&hc), TEMPERATURE_SIGNAL, temperature);
	ADC12IFG = 0;
	ADC12IE = 0;

	/* Turn off the ADC.  ENC first, so the other bits can be altered. */
	ADC12CTL0 &= ~(ENC);
	ADC12CTL0 = 0;

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
#include "bsp-449STK2-temperature-scale.inc"


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
static const char * const calibration_name = "CALIBRATION: ";


/** The length of the name we save in flash.  This must match the length of
    calibration_name (ignoring its terminating null byte.) */
#define CN_LEN 13


/** Base address of flash area.  This is the start of the "information flash"
    in the MSP430F449.  We use the first half (128 bytes) of this area. */
static char * const calibration_base = (char*) 0x1000;


/**
 * Get the calibration from nvram.
 */
int16_t BSP_get_calibration(void)
{
	uint8_t negative;
	uint16_t cal;

	SERIALSTR("get:");

	serial_send(calibration_base);

	if (strncmp(calibration_name, calibration_base, CN_LEN)) {
		SERIALSTR("<A>\r\n");
		return 0;
	}
	if (calibration_base[CN_LEN] == '-') {
		negative = 1;
	} else if (calibration_base[CN_LEN] == '+') {
		negative = 0;
	} else {
		SERIALSTR("<B>0x");
		serial_send_hex_int(calibration_base[CN_LEN]);
		SERIALSTR("\r\n");
		return 0;
	}
	if (calibration_base[CN_LEN+1] < '0'
	    || calibration_base[CN_LEN+1] > '9') {
		SERIALSTR("<C>\r\n");
		return 0;
	}
	if (calibration_base[CN_LEN+2]) {
		SERIALSTR("<D>\r\n");
		return 0;
	}
	if (negative) {
		cal = '0' - calibration_base[CN_LEN+1];
	} else {
		cal = calibration_base[CN_LEN+1] - '0';
	}
	SERIALSTR("\r\n");
	return cal;
}


/**
 * Save the calibration to nvram.
 */
void BSP_save_calibration(int16_t cal)
{
	char *base;
	char s[20];
	char c;
	uint8_t i;

	SERIALSTR("save:");

	Q_ASSERT( cal >= -9 );
	Q_ASSERT( cal <= 9 );


	strcpy(s, calibration_name);
	i = CN_LEN;
	if (cal < 0) {
		s[i++] = '-';
		s[i++] = (((int8_t)cal) * -1) + '0';
	} else {
		s[i++] = '+';
		s[i++] = ((int8_t)cal) + '0';
	}
	s[i++] = '\0';

	/* Do all the flash writing with interrupts off. */
	_BIC_SR(GIE);

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
	_BIC_SR(GIE);
}
