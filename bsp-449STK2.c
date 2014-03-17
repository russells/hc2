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
#include "truefalse.h"


Q_DEFINE_THIS_MODULE("BSP");


#include "bsp-449STK2.h"


#define SB(_port,_bit) do { (_port) |=   (_bit);  } while(0)
#define CB(_port,_bit) do { (_port) &= (~(_bit)); } while(0)


#define TEMPPOWER_OUT P6OUT
#define TEMPPOWER_DIR P6DIR
#define TEMPPOWER_SEL P6SEL
#define TEMPPOWER_BIT BIT0

#define TEMPERATURE_OUT P6OUT
#define TEMPERATURE_DIR P6DIR
#define TEMPERATURE_SEL P6SEL
#define TEMPERATURE_BIT BIT1

#define LED_OUT P2OUT
#define LED_DIR P2DIR
#define LED_SEL P2SEL
#define LED_BIT BIT4

/** Select the temperature analogue input channel. */
#define TEMPERATURE_INCH 0b0001
#define VCC_INCH         0b1011


/** If true, we want QP fast timer 1 running. */
static volatile uint8_t fast_timer_1 = 0;


/** If true, we want QP fast timer 2 running. */
static volatile uint8_t fast_timer_2 = 0;


static struct AssertionBuffer *assertion_buffer;


void Q_onAssert(char const Q_ROM * const Q_ROM_VAR file, int line)
{
	BSP_stop_everything();
	assertion_buffer->valid[0] = '\0';
	assertion_buffer->valid[1] = '\0';
	assertion_buffer->line = line;
	strncpy(assertion_buffer->msg, file, ASSERTION_BUFFER_MSG_SIZE);
	assertion_buffer->msg[ASSERTION_BUFFER_MSG_SIZE - 1] = '\0';
	assertion_buffer->valid[0] = 'A';
	assertion_buffer->valid[1] = 'B';
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
		/* fCLK2/4 == 32Hz */
		(BTIP2 * 0) | (BTIP1 & 0) | BTIP0;
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
}


static void temperature_input_init(void)
{

	/* Ensure the temperature power pin is output as low before we enable
	   it, so we don't get a transient power supply on the chip. */
	CB(TEMPPOWER_OUT, TEMPPOWER_BIT);
	SB(TEMPPOWER_DIR, TEMPPOWER_BIT);
	CB(TEMPPOWER_SEL, TEMPPOWER_BIT); /* IO function. */
	/* Set the temperature analogue as a digital input for now, and only
	   change it to ADC input when required. */
	CB(TEMPERATURE_OUT, TEMPERATURE_BIT);
	CB(TEMPERATURE_DIR, TEMPERATURE_BIT);
	CB(TEMPERATURE_SEL, TEMPERATURE_BIT); /* IO function. */
}


void BSP_init(void)
{
	uint8_t fll_ctl0;

	WDTCTL = WDTPW | WDTHOLD; /* Stop the watchdog */

	FLL_CTL0 &= ~XTS_FLL;	/* XT1 as low-frequency */
	/* Turn on the internal oscillator capacitors.  XCAP18PF is both the
	   XCAPxPF bits on, so use that as a mask for the required bits. */
	fll_ctl0 = FLL_CTL0;
	fll_ctl0 &= ~XCAP18PF;
	fll_ctl0 |= XCAP18PF;
	FLL_CTL0 = fll_ctl0;
	_BIC_SR(OSCOFF);	/* turn on XT1 oscillator */

	do {			/* wait in loop until crystal is stable */
		IFG1 &= ~OFIFG;
	} while (IFG1 & OFIFG);

	// Interrupts on.
	_BIS_SR(GIE);

	basic_timer1_init();

	temperature_input_init();

#ifdef LED
	SB(LED_DIR, LED_BIT);
	BSP_led_off();

	/* Wait at start so we can see resets. */
	BSP_led_on();
	__delay_cycles(1000000L);
	BSP_led_off();
	__delay_cycles(1000000L);
#endif

	/* Buttons are on P1.0 to P1.3. */
	/* Input */
	CB(P1DIR, BIT0);
	CB(P1DIR, BIT1);
	CB(P1DIR, BIT2);
	CB(P1DIR, BIT3);

	/* Set the direction and output bits appropriately for an unused port
	   pin. */
#define UNUSED(port_,bit_)				\
	do {						\
		/* Set to output. */			\
		SB( P ## port_ ## DIR, BIT ## bit_ );	\
		/* Set output low. */			\
		CB( P ## port_ ## OUT, BIT ## bit_ );	\
	} while (0)

	/* Make all the unused pins output zero.  We could set all the bits in
	   each port at once, but this only happens once and doesn't take that
	   long, so it's not an efficiency concern.  Separating them out like
	   this allows us to easily change one pin if it's needed later. */
	UNUSED(1,4);
	UNUSED(1,5);
	UNUSED(1,6);
	UNUSED(1,7);

	UNUSED(2,0);
	UNUSED(2,1);
	UNUSED(2,2);
	UNUSED(2,3);
	//UNUSED(2,4); // LED
	UNUSED(2,5);
	UNUSED(2,6);
	UNUSED(2,7);

	UNUSED(3,0);
	UNUSED(3,1);
	UNUSED(3,2);
	UNUSED(3,3);
	UNUSED(3,4);
	UNUSED(3,5);
	UNUSED(3,6);
	UNUSED(3,7);

	UNUSED(4,1);

	UNUSED(6,2);
	UNUSED(6,3);
	UNUSED(6,4);
	UNUSED(6,5);
	UNUSED(6,6);
	UNUSED(6,7);
}


void QF_onStartup(void)
{
	// Turn on the basic timer interrupt.
	IE2 |= BTIE;
}


void QF_onIdle(void)
{
	BSP_led_off();
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
/**
 * Turn on the monitoring LED.
 *
 * The LED is also used for morse code assertions.
 */
void BSP_led_on(void)
{
	SB(LED_OUT, LED_BIT);
}


/**
 * Turn off the monitoring LED.
 *
 * @see BSP_led_on()
 */
void BSP_led_off(void)
{
	CB(LED_OUT, LED_BIT);
}
#endif


/**
 * Tell the timer ISR to add or subtract an certain number of ticks.
 *
 * The ticks will normally equal zero or plus/minus an eighth of a second.
 *
 * This number is added to the ISR counter when it passes the middle of the
 * count, to avoid any wrapping or over/underflow issues.
 */
static volatile int8_t counter_adjustment = 0;


/**
 * Advance the tick counter to make the second that much shorter.
 *
 * This speeds up (advances) the clock.
 */
void BSP_add_8th_second(void)
{
	Q_ASSERT( ! counter_adjustment );
	__disable_interrupt();
	counter_adjustment = 4;
	__enable_interrupt();
}


/**
 * Retard the tick counter to make the second that much longer.
 *
 * This slows down (retards) the clock.
 */
void BSP_sub_8th_second(void)
{
	Q_ASSERT( ! counter_adjustment );
	__disable_interrupt();
	counter_adjustment = -4;
	__enable_interrupt();
}


static inline uint8_t BSP_button_1(void) { return ! (P1IN & BIT0); }


static inline uint8_t BSP_button_2(void) { return ! (P1IN & BIT1); }


static inline uint8_t BSP_button_3(void) { return ! (P1IN & BIT2); }


static inline uint8_t BSP_button_4(void) { return ! (P1IN & BIT3); }


static volatile uint8_t restart_seconds = FALSE;


/**
 * Make the seconds counter start from zero.
 *
 * This enables the user to set the time accurately.
 */
void BSP_restart_seconds(void)
{
	restart_seconds = TRUE;
}


static void
__attribute__((__interrupt__(BASICTIMER_VECTOR)))
isr_BASICTIMER(void)
{
	BSP_led_on();

	/** We interrupt at fCLK2/4 (ie 32768/(256*4)==32) times per second.
	    We want a slow tick every two seconds, so divide the real tick rate
	    by 64.  This const int is optimised away by the compiler. */
	static const uint8_t COUNT = 64;
	static volatile uint8_t counter = 0;

	/* Wait for this many ticks with all buttons up before reacting to
	   buttons. */
	static const uint8_t BUTTONS_COUNTER_MAX = 5;

	/* The current number of ticks to wait for all buttons up. */
	static volatile uint8_t buttons_counter = BUTTONS_COUNTER_MAX;

	if (restart_seconds) {
		counter = 0;
		restart_seconds = FALSE;
	}

	counter ++;
	if ( ((COUNT/2) == counter) && counter_adjustment ) {
		SERIALSTR("<b:");
		serial_send_int(counter);
		SERIALSTR(":");
		serial_send_int(counter_adjustment);
		SERIALSTR(":");

		counter += counter_adjustment;
		counter_adjustment = 0;

		serial_send_int(counter);
		SERIALSTR(">");
	}
	if (counter >= COUNT) {
		counter = 0;
	}

	uint8_t b1 = BSP_button_1();
	uint8_t b2 = BSP_button_2();
	uint8_t b3 = BSP_button_3();
	uint8_t b4 = BSP_button_4();

	if ( ! (fast_timer_1 || fast_timer_2) ) {

		/* Neither fast timer is on, so check the buttons here. */

		if (buttons_counter) {
			/* We're waiting for all the buttons to be up for this
			   number of ISRs, since the fast timers have just been
			   turned off. */
			if ( ! (b1 || b2 || b3 || b4) ) {
				buttons_counter --;
			} else {
				/* While waiting, we saw a button down, so
				   start the count again. */
				buttons_counter = BUTTONS_COUNTER_MAX;
			}
		}
		else {
			/* Neither fast timer is on, so check the buttons here.  We
			   check buttons 2, 3, and 4.  Button 1 is the cancel button,
			   which is pointless to check in this state. */
			if (b1) {
				SERIALSTR("\\\\1");
				postISR(&ui.super, BUTTON_1_PRESS_SIGNAL, 0);
			} else if (b2) {
				SERIALSTR("\\\\2");
				postISR(&ui.super, BUTTON_2_PRESS_SIGNAL, 0);
			} else if (b3) {
				SERIALSTR("\\\\3");
				postISR(&ui.super, BUTTON_3_PRESS_SIGNAL, 0);
			} else if (b4) {
				SERIALSTR("\\\\4");
				postISR(&ui.super, BUTTON_4_PRESS_SIGNAL, 0);
			}
			if (b1 || b2 || b3 || b4) {
				/* The buttons event will start the fast timer, so tell
				   the buttons state machine to wait for idle buttons
				   before proceeding. */
				postISR(&buttons.super, BUTTONS_WAIT_SIGNAL, 0);
			}
		}
	}
	else {
		/* One or both of the fast timers is currently on, so punt the
		   button event checking to the buttons state machine. */
		QParam buttonmask = 0;
		if (b1) buttonmask |= 0b0001;
		if (b2) buttonmask |= 0b0010;
		if (b3) buttonmask |= 0b0100;
		if (b4) buttonmask |= 0b1000;
 		postISR(&buttons.super, BUTTONS_SIGNAL, buttonmask);

		buttons_counter = BUTTONS_COUNTER_MAX;
	}

	if (fast_timer_1) {
		QF_tickXISR(1);
	}
	if (fast_timer_2) {
		QF_tickXISR(2);
	}

	if (! counter) {
		SERIALSTR("<B>");
		QF_tick();
	}
	EXIT_LPM();
}


void BSP_enable_morse_line(void)
{
	SB(LED_DIR, LED_BIT);
}


void BSP_morse_signal(uint8_t onoff)
{
	if (onoff) {
		SB(LED_OUT, LED_BIT);
	} else {
		CB(LED_OUT, LED_BIT);
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


static volatile QActive *adc_ao;
static volatile QSignal adc_signal;
static volatile uint16_t adc_channel;


uint8_t BSP_start_temperature_reading(QActive *ao, QSignal signal,
				      uint16_t channel)
{
	uint16_t inch_bits;

	__disable_interrupt();
	uint16_t adcon = ADC12CTL0 & ADC12ON;
	__enable_interrupt();
	if (adcon) {
		SERIALSTR("<-A:"); serial_send_int(channel); SERIALSTR(">");
		return FALSE;
	}
	SERIALSTR("<+A:"); serial_send_int(channel); SERIALSTR(">");


	switch (channel) {
	case temperature_channel:
		inch_bits = TEMPERATURE_INCH;
		break;
	case vcc_channel:
		inch_bits = VCC_INCH;
		break;
	default:
		inch_bits = 0;
		Q_ASSERT( 0 );
		break;
	}

	adc_ao = ao;
	adc_signal = signal;
	adc_channel = channel;

	SERIALSTR("t");

	SB(TEMPPOWER_OUT, TEMPPOWER_BIT);     /* Power up the MCP9700A */
	SB(TEMPERATURE_SEL, TEMPERATURE_BIT); /* Analogue function */

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
		/* Input channel */
		| inch_bits;
	/* Reset all pending ADC12 interrupts. */
	ADC12IFG = 0;
	/* No more work here, as we need to wait for the reference to
	   stabilise.  The main QHsm will wiat an appropriate time, and call
	   BSP_get_temperature() to retrieve the reading. */

	/* We were able to use the ADC, so indicate that. */
	return TRUE;
}


void BSP_get_temperature(void)
{
	SERIALSTR("T");

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

	/** @todo move the previous_adc stuff in to recorder.c */

	/** @todo make the BSP convert adc to temperature.  It already does,
	    but make convert_adc_to_temperature() public, and prefix with BSP_.
	 */
	BSP_led_on();

	adc = ADC12MEM10;
	SERIALSTR("<a:");
	serial_send_int(adc);
	SERIALSTR(">");
	/* The ADC value must move by at least four LSB before we regard it as
	   having changed.  This reduces jitter in the temperature readings. */
	if (abs(adc - previous_adc) >= 4) {
		temperature = BSP_convert_adc_to_temperature(adc);
		previous_temperature = temperature;
		previous_adc = adc;
	} else {
		temperature = previous_temperature;
		serial_send_int(previous_adc);
	}
	postISR(adc_ao, adc_signal, temperature);
	ADC12IFG = 0;
	ADC12IE = 0;

	/* Turn off the ADC.  ENC first, so the other bits can be altered. */
	ADC12CTL0 &= ~(ENC);
	ADC12CTL0 = 0;

	CB(TEMPERATURE_OUT, TEMPERATURE_BIT); /* Ensure the temperature pin is
						 0 output before we change it
						 to IO function. */
	CB(TEMPPOWER_OUT, TEMPPOWER_BIT);     /* Power down the MCP9700A */
	SB(TEMPERATURE_SEL, TEMPERATURE_BIT); /* IO function on temperature
						 pin */

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
int16_t BSP_convert_adc_to_temperature(uint16_t adc)
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
    calibration name, plus four bytes for the sign, two digits, and ';'. */
static char * const adjustment_base = ((char *)(0x1000 + CALIBRATION_LEN + 4));


static int16_t get_saved_num(const char * const base,
			     const char *const name, uint8_t len)
{
	uint8_t negative;
	uint16_t num;

	SERIALSTR("get:");

	//serial_send(base);

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
	if (base[len+2] < '0'
	    || base[len+2] > '9') {
		SERIALSTR("<D>\r\n");
		return 0;
	}
	if (base[len+3] != ';') {
		SERIALSTR("<E>\r\n");
		return 0;
	}
	num = 10 * (base[len+1] - '0') + (base[len+2] - '0');
	if (negative) {
		num = -num;
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

	Q_ASSERT( cal >= MIN_CAL );
	Q_ASSERT( cal <= MAX_CAL );
	Q_ASSERT( adj >= MIN_ADJ );
	Q_ASSERT( adj <= MAX_ADJ );

	slen = snprintf(s, 50, "%s%+03d;%s%+03d;",
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


void BSP_set_assertion_buffer(struct AssertionBuffer *buf)
{
	assertion_buffer = buf;
	if (assertion_buffer->valid[0] != 'A'
	    || assertion_buffer->valid[1] != 'B') {
		assertion_buffer->valid[0] = '\0';
		assertion_buffer->valid[1] = '\0';
		assertion_buffer->line = 0;
		for (uint8_t i=0; i< ASSERTION_BUFFER_MSG_SIZE; i++) {
			assertion_buffer->msg[i] = '\0';
		}
	}
}


const struct AssertionBuffer *BSP_get_assertion_buffer(void)
{
	return assertion_buffer;
}
