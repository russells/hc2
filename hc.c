
#include "qpn_port.h"
#include <inttypes.h>
#include <string.h>
#include "hc.h"
#include "bsp.h"
#include "lcd.h"
#include "serial.h"


Q_DEFINE_THIS_FILE;


struct Hc hc;

static void hc_ctor(void);
static QState hcInitial(struct Hc *me);
static QState hcTop(struct Hc *me);
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

QActiveCB const Q_ROM Q_ROM_VAR QF_active[] = {
	{ (QActive*)0           , (QEvent*)0   , 0                   },
	{ (QActive *) (&hc)     , hcQueue      , Q_DIM(hcQueue)      },
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
	SERIALSTR("Let's go...\r\n");
	QF_run();
	goto startmain;

	return 0;
}


static void hc_ctor(void)
{
	QActive_ctor((QActive*)(&hc), (QStateHandler)(&hcInitial));
	hc.ti = INVALIDTI;
}


static QState hcInitial(struct Hc *me)
{
	return Q_TRAN(hcTemperature);
}


static QState hcTop(struct Hc *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		lcd_clear();
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


static QState hcCalibrate(struct Hc *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		BSP_fast_timer();
		return Q_HANDLED();
	}
	return Q_SUPER(hcTop);
}


static QState hcCalibratePause(struct Hc *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		QActive_arm((QActive*)me, 6);
		return Q_HANDLED();
	case Q_TIMEOUT_SIG:
		if (! BSP_switch_pressed()) {
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
		BSP_fast_timer();
		BSP_start_temperature_reading();
		QActive_arm((QActive*)me, 2);
		return Q_HANDLED();
	case Q_TIMEOUT_SIG:
		return Q_TRAN(hcCalibrateGetTemperature);
	}
	return Q_SUPER(hcCalibrate);
}


static QState hcCalibrateGetTemperature(struct Hc *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		BSP_get_temperature();
		BSP_fast_timer(); /* This resets the timer, so we get two
				     complete ticks. */
		QActive_arm((QActive*)me, 2);
		return Q_HANDLED();
	case TEMPERATURE_SIGNAL:
		me->ti = Q_PAR(me);
		show_temperature_cal(me);
		return Q_TRAN(hcCalibratePause);
	case Q_TIMEOUT_SIG:
		return Q_TRAN(hcCalibratePause);
	}
	return Q_SUPER(hcCalibrateTemperature);
}


static QState hcPause(struct Hc *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		SERIALSTR("hcPause\r\n");
		BSP_slow_timer();
		QActive_arm((QActive*)me, 1);
		return Q_HANDLED();
	case Q_TIMEOUT_SIG:
		if (BSP_switch_pressed()) {
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
		BSP_fast_timer();
		// Start reading the temperature.
		BSP_start_temperature_reading();
		// If we time out, go back to just waiting.
		QActive_arm((QActive*)me, 1);
		return Q_HANDLED();
	case Q_TIMEOUT_SIG:
		return Q_TRAN(hcGetTemperature);
	}
	return Q_SUPER(hcTop);
}


static QState hcGetTemperature(struct Hc *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		SERIALSTR("hcGet\r\n");
		BSP_get_temperature();
		QActive_arm((QActive*)me, 2);
		return Q_HANDLED();
	case TEMPERATURE_SIGNAL:
		me->ti = (int16_t) Q_PAR(me);
		show_temperature(me);
		return Q_TRAN(hcPause);
	case Q_TIMEOUT_SIG:
		return Q_TRAN(hcPause);
	}
	return Q_SUPER(hcTemperature);
}


/* Contains declarations of temperature sensing ranges: MINTI, MAXTI,
   NCONVERSIONS. */
#include "bsp-temperature-scale.h"


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
	uint16_t ti;
	const char *tstring;

	if (LOWTI == me->ti) {
		SERIALSTR("LOW: ");
		serial_send_int(me->ti);
		SERIALSTR("\r\n");
		lcd_clear();
		lcd_showstring(lowtistring);
		return;
	}
	if (HIGHTI == me->ti) {
		SERIALSTR("HIGH: ");
		serial_send_int(me->ti);
		SERIALSTR("\r\n");
		lcd_clear();
		lcd_showstring(hightistring);
		return;
	}

	ti = me->ti;
	SERIALSTR("ti: ");
	serial_send_int(ti);
	SERIALSTR(":");
	ti -= MINTI;		/* Move the scale up to zero-based. */
	Q_ASSERT( ti >= 0 );	/* Range checking. */
	Q_ASSERT( ti < NCONVERSIONS );
	ti /= 2;		/* Scale to whole degrees. */
	serial_send_int(ti);
	SERIALSTR("\"");
	tstring = tstrings[ti];
	serial_send(tstring);
	SERIALSTR("\"\r\n");
	lcd_showstring(tstring);
}


const char tstrings_cal[202][8] = {
	"-3\xb0" "0 C",
	"-2\xb9" "5 C", "-2\xb9" "0 C", "-2\xb8" "5 C", "-2\xb8" "0 C",
	"-2\xb7" "5 C", "-2\xb7" "0 C", "-2\xb6" "5 C", "-2\xb6" "0 C",
	"-2\xb5" "5 C", "-2\xb5" "0 C", "-2\xb4" "5 C", "-2\xb4" "0 C",
	"-2\xb3" "5 C", "-2\xb3" "0 C", "-2\xb2" "5 C", "-2\xb2" "0 C",
	"-2\xb1" "5 C", "-2\xb1" "0 C", "-2\xb0" "5 C", "-2\xb0" "0 C",
	"-1\xb9" "5 C", "-1\xb9" "0 C", "-1\xb8" "5 C", "-1\xb8" "0 C",
	"-1\xb7" "5 C", "-1\xb7" "0 C", "-1\xb6" "5 C", "-1\xb6" "0 C",
	"-1\xb5" "5 C", "-1\xb5" "0 C", "-1\xb4" "5 C", "-1\xb4" "0 C",
	"-1\xb3" "5 C", "-1\xb3" "0 C", "-1\xb2" "5 C", "-1\xb2" "0 C",
	"-1\xb1" "5 C", "-1\xb1" "0 C", "-1\xb0" "5 C", "-1\xb0" "0 C",
	"- \xb9" "5 C", "- \xb9" "0 C", "- \xb8" "5 C", "- \xb8" "0 C",
	"- \xb7" "5 C", "- \xb7" "0 C", "- \xb6" "5 C", "- \xb6" "0 C",
	"- \xb5" "5 C", "- \xb5" "0 C", "- \xb4" "5 C", "- \xb4" "0 C",
	"- \xb3" "5 C", "- \xb3" "0 C", "- \xb2" "5 C", "- \xb2" "0 C",
	"- \xb1" "5 C", "- \xb1" "0 C", "- \xb0" "5 C",
	"+ \xb0" "0 C", "+ \xb0" "5 C", "+ \xb1" "0 C", "+ \xb1" "5 C",
	"+ \xb2" "0 C", "+ \xb2" "5 C", "+ \xb3" "0 C", "+ \xb3" "5 C",
	"+ \xb4" "0 C", "+ \xb4" "5 C", "+ \xb5" "0 C", "+ \xb5" "5 C",
	"+ \xb6" "0 C", "+ \xb6" "5 C", "+ \xb7" "0 C", "+ \xb7" "5 C",
	"+ \xb8" "0 C", "+ \xb8" "5 C", "+ \xb9" "0 C", "+ \xb9" "5 C",
	"+1\xb0" "0 C", "+1\xb0" "5 C", "+1\xb1" "0 C", "+1\xb1" "5 C",
	"+1\xb2" "0 C", "+1\xb2" "5 C", "+1\xb3" "0 C", "+1\xb3" "5 C",
	"+1\xb4" "0 C", "+1\xb4" "5 C", "+1\xb5" "0 C", "+1\xb5" "5 C",
	"+1\xb6" "0 C", "+1\xb6" "5 C", "+1\xb7" "0 C", "+1\xb7" "5 C",
	"+1\xb8" "0 C", "+1\xb8" "5 C", "+1\xb9" "0 C", "+1\xb9" "5 C",
	"+2\xb0" "0 C", "+2\xb0" "5 C", "+2\xb1" "0 C", "+2\xb1" "5 C",
	"+2\xb2" "0 C", "+2\xb2" "5 C", "+2\xb3" "0 C", "+2\xb3" "5 C",
	"+2\xb4" "0 C", "+2\xb4" "5 C", "+2\xb5" "0 C", "+2\xb5" "5 C",
	"+2\xb6" "0 C", "+2\xb6" "5 C", "+2\xb7" "0 C", "+2\xb7" "5 C",
	"+2\xb8" "0 C", "+2\xb8" "5 C", "+2\xb9" "0 C", "+2\xb9" "5 C",
	"+3\xb0" "0 C", "+3\xb0" "5 C", "+3\xb1" "0 C", "+3\xb1" "5 C",
	"+3\xb2" "0 C", "+3\xb2" "5 C", "+3\xb3" "0 C", "+3\xb3" "5 C",
	"+3\xb4" "0 C", "+3\xb4" "5 C", "+3\xb5" "0 C", "+3\xb5" "5 C",
	"+3\xb6" "0 C", "+3\xb6" "5 C", "+3\xb7" "0 C", "+3\xb7" "5 C",
	"+3\xb8" "0 C", "+3\xb8" "5 C", "+3\xb9" "0 C", "+3\xb9" "5 C",
	"+4\xb0" "0 C", "+4\xb0" "5 C", "+4\xb1" "0 C", "+4\xb1" "5 C",
	"+4\xb2" "0 C", "+4\xb2" "5 C", "+4\xb3" "0 C", "+4\xb3" "5 C",
	"+4\xb4" "0 C", "+4\xb4" "5 C", "+4\xb5" "0 C", "+4\xb5" "5 C",
	"+4\xb6" "0 C", "+4\xb6" "5 C", "+4\xb7" "0 C", "+4\xb7" "5 C",
	"+4\xb8" "0 C", "+4\xb8" "5 C", "+4\xb9" "0 C", "+4\xb9" "5 C",
	"+5\xb0" "0 C", "+5\xb0" "5 C", "+5\xb1" "0 C", "+5\xb1" "5 C",
	"+5\xb2" "0 C", "+5\xb2" "5 C", "+5\xb3" "0 C", "+5\xb3" "5 C",
	"+5\xb4" "0 C", "+5\xb4" "5 C", "+5\xb5" "0 C", "+5\xb5" "5 C",
	"+5\xb6" "0 C", "+5\xb6" "5 C", "+5\xb7" "0 C", "+5\xb7" "5 C",
	"+5\xb8" "0 C", "+5\xb8" "5 C", "+5\xb9" "0 C", "+5\xb9" "5 C",
	"+6\xb0" "0 C", "+6\xb0" "5 C", "+6\xb1" "0 C", "+6\xb1" "5 C",
	"+6\xb2" "0 C", "+6\xb2" "5 C", "+6\xb3" "0 C", "+6\xb3" "5 C",
	"+6\xb4" "0 C", "+6\xb4" "5 C", "+6\xb5" "0 C", "+6\xb5" "5 C",
	"+6\xb6" "0 C", "+6\xb6" "5 C", "+6\xb7" "0 C", "+6\xb7" "5 C",
	"+6\xb8" "0 C", "+6\xb8" "5 C", "+6\xb9" "0 C", "+6\xb9" "5 C",
	"+7\xb0" "0 C", "+7\xb5" "5 C",
};


static void show_temperature_cal(struct Hc *me)
{
	int16_t ti;
	const char *tstring;

	if (LOWTI == me->ti) {
		lcd_clear();
		lcd_showstring(lowtistring);
		return;
	}
	if (HIGHTI == me->ti) {
		lcd_clear();
		lcd_showstring(hightistring);
		return;
	}
	ti = me->ti;
	ti -= MINTI;
	Q_ASSERT( ti >= 0 );
	Q_ASSERT( ti < NCONVERSIONS );
	tstring = tstrings_cal[ti];

	{
		SERIALSTR("cal ");
		serial_send_int(me->ti);
		SERIALSTR(":");
		serial_send_int(ti);
		SERIALSTR(" \"");
		for (uint8_t i = 0; tstring[i]; i++) {
			serial_send_char(0x7f & tstring[i]);
			if (0x80 & tstring[i]) {
				serial_send_char('.');
			}
		}
		SERIALSTR("\"\r\n");
	}


	lcd_clear();
	lcd_showstring(tstring);
}
