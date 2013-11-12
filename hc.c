
#include "qpn_port.h"
#include <inttypes.h>
#include <string.h>
#include "hc.h"
#include "bsp.h"
#include "lcd.h"
#include "rtc.h"
#include "serial.h"
/* Contains declarations of temperature sensing ranges: MINTI, MAXTI,
   NCONVERSIONS. */
#include "bsp-temperature-scale.h"


Q_DEFINE_THIS_FILE;


/**
 * The startup banner message.
 *
 * Starts with seven spaces so the strings scrolls across starting at the
 * right.  This avoids special case handling of the beginning of the string.
 */
static const char banner[] = "       \"HOTS'N'COLDS\" - FOR ANDREW AND KATE";


struct Hc hc;

static void hc_ctor(void);
static QState hcInitial(struct Hc *me);
static QState hcTop(struct Hc *me);

static QState scroll(struct Hc *me);
static QState scrollText(struct Hc *me);

static QState hcPause(struct Hc *me);
static QState hcTemperature(struct Hc *me);
static QState hcGetTemperature(struct Hc *me);
static QState hcCalibrate(struct Hc *me);
static QState hcCalibratePause(struct Hc *me);
static QState hcCalibrateTemperature(struct Hc *me);
static QState hcCalibrateGetTemperature(struct Hc *me);

static void show_temperature(struct Hc *me);
static void show_temperature_cal(struct Hc *me);


static QEvent hcQueue[4];
static QEvent rtcQueue[4];

QActiveCB const Q_ROM Q_ROM_VAR QF_active[] = {
	{ (QActive*)0           , (QEvent*)0   , 0                   },
	{ (QActive *) (&hc)     , hcQueue      , Q_DIM(hcQueue)      },
	{ (QActive *) (&rtc)    , rtcQueue     , Q_DIM(rtcQueue)     },
};
Q_ASSERT_COMPILE( QF_MAX_ACTIVE == Q_DIM(QF_active) - 1 );


int main(void)
{
 startmain:
	BSP_init();
	serial_init();
	SERIALSTR("\r\n\r\n*** Hots and Colds ***\r\n");
	lcd_init();
	hc_ctor();
	rtc_ctor();
	SERIALSTR("Let's go...\r\n");
	QF_run();
	goto startmain;

	return 0;
}


static void hc_ctor(void)
{
	QActive_ctor((QActive*)(&hc), (QStateHandler)(&hcInitial));
	hc.ti = INVALIDTI;
	hc.calibration = BSP_get_calibration();
}


static QState hcInitial(struct Hc *me)
{
	return Q_TRAN(scrollText);
}


static QState hcTop(struct Hc *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		lcd_clear();
		lcd_showdigits("6789");
		break;
	case WATCHDOG_SIGNAL:
		// TODO: watchdog reset
		return Q_HANDLED();
	case TEMPERATURE_SIGNAL:
		SERIALSTR("hcTop: TS\r\n");
		return Q_HANDLED();
	}
	return Q_SUPER(QHsm_top);
}


static QState scroll(struct Hc *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		me->scrollstring = banner;
		me->scrollindex = 0;
		BSP_fast_timer(1);
		return Q_HANDLED();
	}
	return Q_SUPER(hcTop);
}


static QState scrollText(struct Hc *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		strncpy(me->scrolltext, me->scrollstring+me->scrollindex, 7);
		me->scrolltext[7] = '\0';
		lcd_showstring(me->scrolltext);
		QActive_armX((QActive*)me, 1, 15);
		return Q_HANDLED();
	case Q_TIMEOUT1_SIG:
		me->scrollindex ++;
		if (me->scrollstring[me->scrollindex]) {
			return Q_TRAN(scrollText);
		} else {
			return Q_TRAN(hcTemperature);
		}
	}
	return Q_SUPER(scroll);
}


static QState hcCalibrate(struct Hc *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		BSP_fast_timer(1);
		return Q_HANDLED();
	case Q_EXIT_SIG:
		BSP_save_calibration(me->calibration);
		/* Save this as an invalid value, so at the next tick
		   hcGetTemperature() will be forced to update the display. */
		me->ti = INVALIDTI;
		return Q_HANDLED();
	}
	return Q_SUPER(hcTop);
}


static QState hcCalibratePause(struct Hc *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		QActive_armX((QActive*)me, 1, 25);
		return Q_HANDLED();
	case Q_TIMEOUT1_SIG:
		if (! BSP_cal_switch()) {
			SERIALSTR("hcCP no sw\r\n");
			return Q_TRAN(hcTemperature);
		} else {
			return Q_TRAN(hcCalibrateTemperature);
		}
	}
	return Q_SUPER(hcCalibrate);
}


static QState hcCalibrateTemperature(struct Hc *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		SERIALSTR("hcCT\r\n");
		BSP_start_temperature_reading();
		/* The short ticks in the BSP are especially designed so we
		   need only one tick to allow the ADC voltage reference to
		   stabilise. */
		BSP_fast_timer(1);
		QActive_armX((QActive*)me, 1, 1);
		return Q_HANDLED();
	case Q_TIMEOUT1_SIG:
		return Q_TRAN(hcCalibrateGetTemperature);
	}
	return Q_SUPER(hcCalibrate);
}


static QState hcCalibrateGetTemperature(struct Hc *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		BSP_get_temperature();
		BSP_fast_timer(1); /* This resets the timer, so we get two
				      complete ticks. */
		QActive_armX((QActive*)me, 1, 2);
		return Q_HANDLED();
	case TEMPERATURE_SIGNAL:
		me->ti = Q_PAR(me);
		if (BSP_up_switch() && (me->calibration < MAX_CAL)) {
			SERIALSTR("up\r\n");
			me->calibration ++;
		} else if (BSP_down_switch() && (me->calibration > MIN_CAL)) {
			SERIALSTR("down\r\n");
			me->calibration --;
		}
		show_temperature_cal(me);
		return Q_TRAN(hcCalibratePause);
	case Q_TIMEOUT1_SIG:
		return Q_TRAN(hcCalibratePause);
	}
	return Q_SUPER(hcCalibrateTemperature);
}


static QState hcPause(struct Hc *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		SERIALSTR("hcPause\r\n");
		BSP_fast_timer(0);
		QActive_arm((QActive*)me, 1);
		return Q_HANDLED();
	case Q_TIMEOUT_SIG:
		if (BSP_cal_switch()) {
			return Q_TRAN(hcCalibrateTemperature);
		} else {
			return Q_TRAN(hcTemperature);
		}
	}
	return Q_SUPER(hcTop);
}


static QState hcTemperature(struct Hc *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		SERIALSTR("hcTemp\r\n");
		BSP_fast_timer(1);
		// Start reading the temperature.
		BSP_start_temperature_reading();
		// If we time out, go back to just waiting.
		QActive_armX((QActive*)me, 1, 1);
		return Q_HANDLED();
	case Q_TIMEOUT1_SIG:
		return Q_TRAN(hcGetTemperature);
	}
	return Q_SUPER(hcTop);
}


static QState hcGetTemperature(struct Hc *me)
{
	int16_t ti;

	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		SERIALSTR("hcGet\r\n");
		BSP_get_temperature();
		QActive_armX((QActive*)me, 1, 2);
		return Q_HANDLED();
	case TEMPERATURE_SIGNAL:
		ti = (int16_t) Q_PAR(me);
		/* Only update the display if the temperature has changed. */
		if (INVALIDTI == me->ti || ti != me->ti) {
			me->ti = ti;
			show_temperature(me);
		}
		return Q_TRAN(hcPause);
	case Q_TIMEOUT1_SIG:
		return Q_TRAN(hcPause);
	}
	return Q_SUPER(hcTemperature);
}



static int16_t get_calibrated_ti(struct Hc *me)
{
	int16_t adjti;

	SERIALSTR("adj:");
	serial_send_int(me->ti);
	SERIALSTR(":");
	/* If the temperature value is outside the BSP range, leave it
	   there. */
	if ((LOWTI == me->ti) || (HIGHTI == me->ti)) {
		adjti = me->ti;
	} else {
		adjti = me->ti + me->calibration;
		serial_send_int(adjti);
		/* If the calibration moved the value outside the
		   allowed range, indicate that. */
		if (adjti < MINTI) {
			adjti = LOWTI;
		} else if (adjti > MAXTI) {
			adjti = HIGHTI;
		}
	}
	SERIALSTR("\r\n");
	return adjti;
}


/**
 * The strings that are presented to the user for each temperature.
 *
 * The number of strings in here must be exactly half the possible values of
 * the temperature scale, because we measure temperature to half a degree, and
 * present it as a whole degree.
 *
 * This array is not automatically generated because it is independent of the
 * temperature sensor and the ADC in the BSP.
 */
static const char tstrings[101][8] = {
	"50COLDS", "49COLDS", "48COLDS", "47COLDS", "46COLDS",
	"45COLDS", "44COLDS", "43COLDS", "42COLDS", "41COLDS",
	"40COLDS", "39COLDS", "38COLDS", "37COLDS", "36COLDS",
	"35COLDS", "34COLDS", "33COLDS", "32COLDS", "31COLDS",
	"30COLDS", "29COLDS", "28COLDS", "27COLDS", "26COLDS",
	"25COLDS", "24COLDS", "23COLDS", "22COLDS", "21COLDS",
	"20COLDS", "19COLDS", "18COLDS", "17COLDS", "16COLDS",
	"15COLDS", "14COLDS", "13COLDS", "12COLDS", "11COLDS",
	"10COLDS", "9 COLDS", "8 COLDS", "7 COLDS", "6 COLDS",
	"5 COLDS", "4 COLDS", "3 COLDS", "2 COLDS", "1 COLD ",
	" NORMAL",
	"1  HOT ", "2  HOTS", "3  HOTS", "4  HOTS", "5  HOTS",
	"6  HOTS", "7  HOTS", "8  HOTS", "9  HOTS", "10 HOTS",
	"11 HOTS", "12 HOTS", "13 HOTS", "14 HOTS", "15 HOTS",
	"16 HOTS", "17 HOTS", "18 HOTS", "19 HOTS", "20 HOTS",
	"21 HOTS", "22 HOTS", "23 HOTS", "24 HOTS", "25 HOTS",
	"26 HOTS", "27 HOTS", "28 HOTS", "29 HOTS", "30 HOTS",
	"31 HOTS", "32 HOTS", "33 HOTS", "34 HOTS", "35 HOTS",
	"36 HOTS", "37 HOTS", "38 HOTS", "39 HOTS", "40 HOTS",
	"41 HOTS", "42 HOTS", "43 HOTS", "44 HOTS", "45 HOTS",
	"46 HOTS", "47 HOTS", "48 HOTS", "49 HOTS", "50 HOTS",
};

const char lowtistring[] = " ---";
const char hightistring[] = " +++";


static void show_temperature(struct Hc *me)
{
	int16_t adjti;
	const char *tstring;

	adjti = get_calibrated_ti(me);

	if (LOWTI == adjti) {
		SERIALSTR("LOW: ");
		serial_send_int(adjti);
		SERIALSTR("\r\n");
		lcd_clear();
		lcd_showstring(lowtistring);
		return;
	}
	if (HIGHTI == adjti) {
		SERIALSTR("HIGH: ");
		serial_send_int(adjti);
		SERIALSTR("\r\n");
		lcd_clear();
		lcd_showstring(hightistring);
		return;
	}

	SERIALSTR("adjti: ");
	serial_send_int(adjti);
	SERIALSTR(":");
	adjti -= MINTI;		/* Move the scale up to zero-based. */
	Q_ASSERT( adjti >= 0 );	/* Range checking. */
	Q_ASSERT( adjti < NCONVERSIONS );
	adjti /= 2;		/* Scale to whole degrees. */
	serial_send_int(adjti);
	SERIALSTR("\"");
	tstring = tstrings[adjti];
	serial_send(tstring);
	SERIALSTR("\"\r\n");
	lcd_showstring(tstring);
}


const char tstrings_cal[202][5] = {
	"-3\xb0" "0",
	"-2\xb9" "5", "-2\xb9" "0", "-2\xb8" "5", "-2\xb8" "0",
	"-2\xb7" "5", "-2\xb7" "0", "-2\xb6" "5", "-2\xb6" "0",
	"-2\xb5" "5", "-2\xb5" "0", "-2\xb4" "5", "-2\xb4" "0",
	"-2\xb3" "5", "-2\xb3" "0", "-2\xb2" "5", "-2\xb2" "0",
	"-2\xb1" "5", "-2\xb1" "0", "-2\xb0" "5", "-2\xb0" "0",
	"-1\xb9" "5", "-1\xb9" "0", "-1\xb8" "5", "-1\xb8" "0",
	"-1\xb7" "5", "-1\xb7" "0", "-1\xb6" "5", "-1\xb6" "0",
	"-1\xb5" "5", "-1\xb5" "0", "-1\xb4" "5", "-1\xb4" "0",
	"-1\xb3" "5", "-1\xb3" "0", "-1\xb2" "5", "-1\xb2" "0",
	"-1\xb1" "5", "-1\xb1" "0", "-1\xb0" "5", "-1\xb0" "0",
	"- \xb9" "5", "- \xb9" "0", "- \xb8" "5", "- \xb8" "0",
	"- \xb7" "5", "- \xb7" "0", "- \xb6" "5", "- \xb6" "0",
	"- \xb5" "5", "- \xb5" "0", "- \xb4" "5", "- \xb4" "0",
	"- \xb3" "5", "- \xb3" "0", "- \xb2" "5", "- \xb2" "0",
	"- \xb1" "5", "- \xb1" "0", "- \xb0" "5",
	"+ \xb0" "0", "+ \xb0" "5", "+ \xb1" "0", "+ \xb1" "5",
	"+ \xb2" "0", "+ \xb2" "5", "+ \xb3" "0", "+ \xb3" "5",
	"+ \xb4" "0", "+ \xb4" "5", "+ \xb5" "0", "+ \xb5" "5",
	"+ \xb6" "0", "+ \xb6" "5", "+ \xb7" "0", "+ \xb7" "5",
	"+ \xb8" "0", "+ \xb8" "5", "+ \xb9" "0", "+ \xb9" "5",
	"+1\xb0" "0", "+1\xb0" "5", "+1\xb1" "0", "+1\xb1" "5",
	"+1\xb2" "0", "+1\xb2" "5", "+1\xb3" "0", "+1\xb3" "5",
	"+1\xb4" "0", "+1\xb4" "5", "+1\xb5" "0", "+1\xb5" "5",
	"+1\xb6" "0", "+1\xb6" "5", "+1\xb7" "0", "+1\xb7" "5",
	"+1\xb8" "0", "+1\xb8" "5", "+1\xb9" "0", "+1\xb9" "5",
	"+2\xb0" "0", "+2\xb0" "5", "+2\xb1" "0", "+2\xb1" "5",
	"+2\xb2" "0", "+2\xb2" "5", "+2\xb3" "0", "+2\xb3" "5",
	"+2\xb4" "0", "+2\xb4" "5", "+2\xb5" "0", "+2\xb5" "5",
	"+2\xb6" "0", "+2\xb6" "5", "+2\xb7" "0", "+2\xb7" "5",
	"+2\xb8" "0", "+2\xb8" "5", "+2\xb9" "0", "+2\xb9" "5",
	"+3\xb0" "0", "+3\xb0" "5", "+3\xb1" "0", "+3\xb1" "5",
	"+3\xb2" "0", "+3\xb2" "5", "+3\xb3" "0", "+3\xb3" "5",
	"+3\xb4" "0", "+3\xb4" "5", "+3\xb5" "0", "+3\xb5" "5",
	"+3\xb6" "0", "+3\xb6" "5", "+3\xb7" "0", "+3\xb7" "5",
	"+3\xb8" "0", "+3\xb8" "5", "+3\xb9" "0", "+3\xb9" "5",
	"+4\xb0" "0", "+4\xb0" "5", "+4\xb1" "0", "+4\xb1" "5",
	"+4\xb2" "0", "+4\xb2" "5", "+4\xb3" "0", "+4\xb3" "5",
	"+4\xb4" "0", "+4\xb4" "5", "+4\xb5" "0", "+4\xb5" "5",
	"+4\xb6" "0", "+4\xb6" "5", "+4\xb7" "0", "+4\xb7" "5",
	"+4\xb8" "0", "+4\xb8" "5", "+4\xb9" "0", "+4\xb9" "5",
	"+5\xb0" "0", "+5\xb0" "5", "+5\xb1" "0", "+5\xb1" "5",
	"+5\xb2" "0", "+5\xb2" "5", "+5\xb3" "0", "+5\xb3" "5",
	"+5\xb4" "0", "+5\xb4" "5", "+5\xb5" "0", "+5\xb5" "5",
	"+5\xb6" "0", "+5\xb6" "5", "+5\xb7" "0", "+5\xb7" "5",
	"+5\xb8" "0", "+5\xb8" "5", "+5\xb9" "0", "+5\xb9" "5",
	"+6\xb0" "0", "+6\xb0" "5", "+6\xb1" "0", "+6\xb1" "5",
	"+6\xb2" "0", "+6\xb2" "5", "+6\xb3" "0", "+6\xb3" "5",
	"+6\xb4" "0", "+6\xb4" "5", "+6\xb5" "0", "+6\xb5" "5",
	"+6\xb6" "0", "+6\xb6" "5", "+6\xb7" "0", "+6\xb7" "5",
	"+6\xb8" "0", "+6\xb8" "5", "+6\xb9" "0", "+6\xb9" "5",
	"+7\xb0" "0", "+7\xb5" "5",
};


static const char calstrings[19][4] = {
	"-\xb4" "5", "-\xb4" "0", "-\xb3" "5", "-\xb3" "0",
	"-\xb2" "5", "-\xb2" "0", "-\xb1" "5", "-\xb1" "0",
	"-\xb0" "5", " \xb0" "0", "+\xb0" "5",
	"+\xb1" "0", "+\xb1" "5", "+\xb2" "0", "+\xb2" "5",
	"+\xb3" "0", "+\xb3" "5", "+\xb4" "0", "+\xb4" "5",
};


static void show_temperature_cal(struct Hc *me)
{
	int16_t adjti;
	char s[8];		/* TODO change to s[9] when we get the bigger
				   LCD.  Also change the layout code below. */

	adjti = get_calibrated_ti(me);

	if (LOWTI == adjti) {
		lcd_clear();
		lcd_showstring(lowtistring);
		return;
	}
	if (HIGHTI == adjti) {
		lcd_clear();
		lcd_showstring(hightistring);
		return;
	}
	adjti -= MINTI;
	Q_ASSERT( adjti >= 0 );
	Q_ASSERT( adjti < NCONVERSIONS );

	strcpy(s, tstrings_cal[adjti]);

	Q_ASSERT( me->calibration >= MIN_CAL );
	Q_ASSERT( me->calibration <= MAX_CAL );

	strcpy(s+4, calstrings[ me->calibration - MIN_CAL ]);

	{
		SERIALSTR("cal ");
		serial_send_int(adjti);
		SERIALSTR(":");
		serial_send_int(adjti);
		SERIALSTR(" \"");
		for (uint8_t i = 0; s[i]; i++) {
			serial_send_char(0x7f & s[i]);
			if (0x80 & s[i]) {
				serial_send_char('.');
			}
		}
		SERIALSTR("\"\r\n");
	}


	lcd_clear();
	lcd_showstring(s);
}
