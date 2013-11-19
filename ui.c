#include "ui.h"
#include "hc.h"
#include "bsp.h"
#include "serial.h"
#include "lcd.h"
#include "rtc.h"
#include "time.h"
#include <string.h>
/* Contains declarations of temperature sensing ranges: MINTI, MAXTI,
   NCONVERSIONS. */
#include "bsp-temperature-scale.h"


#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif


Q_DEFINE_THIS_MODULE("u");


static QState uiInitial(struct UI *me);
static QState uiTop(struct UI *me);

static QState scroll(struct UI *me);
static QState scrollText(struct UI *me);

static QState uiRun(struct UI *me);
static QState uiPause(struct UI *me);
static QState uiTemperature(struct UI *me);
static QState uiGetTemperature(struct UI *me);

static QState uiMenu(struct UI *me);

static QState uiMenuMaybeSettime(struct UI *me);
static QState uiMenuSettimeHours(struct UI *me);
static QState uiMenuSettimeHoursFlash(struct UI *me);
static QState uiMenuSettimeMinutes(struct UI *me);
static QState uiMenuSettimeMinutesFlash(struct UI *me);
static QState uiMenuSettimeConfirmYes(struct UI *me);
static QState uiMenuSettimeConfirmNo(struct UI *me);

static QState uiMenuMaybeCalibrate(struct UI *me);
static QState uiMenuCalibratePause(struct UI *me);
static QState uiMenuCalibrateTemperature(struct UI *me);
static QState uiMenuCalibrateGetTemperature(struct UI *me);

static QState uiMenuMaybeExit(struct UI *me);

static void show_temperature(struct UI *me);
static void show_temperature_cal(struct UI *me);


/**
 * The startup banner message.
 *
 * Starts with seven spaces so the strings scrolls across starting at the
 * right.  This avoids special case handling of the beginning of the string.
 */
static const char banner[] = "       \"HOTS'N'COLDS\" - FOR ANDREW AND KATE";


struct UI ui;


void ui_ctor(void)
{
	QActive_ctor((QActive*)(&ui), (QStateHandler)(&uiInitial));
	ui.ti = INVALIDTI;
	ui.calibration = BSP_get_calibration();
}


static QState uiInitial(struct UI *me)
{
	return Q_TRAN(scrollText);
}


static QState uiTop(struct UI *me)
{
	const struct Time *time;

	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		lcd_clear();
		lcd_showdigits("6789");
		break;
	case WATCHDOG_SIGNAL:
		// TODO: watchdog reset
		return Q_HANDLED();
	case TEMPERATURE_SIGNAL:
		SERIALSTR("uiTop: TS\r\n");
		return Q_HANDLED();
	case TIME_SIGNAL:
		time = (const struct Time*)(Q_PAR(me));
		Q_ASSERT( time->ht >= '0' && time->ht <= '9' );
		Q_ASSERT( time->h1 >= '0' && time->h1 <= '9' );
		Q_ASSERT( time->mt >= '0' && time->mt <= '9' );
		Q_ASSERT( time->m1 >= '0' && time->m1 <= '9' );
		lcd_showdigits((const char *)time);
		if (time->seconds & 0x02) {
			lcd_colon(1);
		} else {
			lcd_colon(0);
		}
		return Q_HANDLED();
	}
	return Q_SUPER(QHsm_top);
}


static QState scroll(struct UI *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		me->scrollstring = banner;
		me->scrollindex = 0;
		BSP_fast_timers(1, 0);
		return Q_HANDLED();
	case BUTTON_1_PRESS_SIGNAL:
	case BUTTON_1_LONG_PRESS_SIGNAL:
	case BUTTON_1_REPEAT_SIGNAL:
	case BUTTON_2_PRESS_SIGNAL:
	case BUTTON_2_LONG_PRESS_SIGNAL:
	case BUTTON_2_REPEAT_SIGNAL:
	case BUTTON_3_PRESS_SIGNAL:
	case BUTTON_3_LONG_PRESS_SIGNAL:
	case BUTTON_3_REPEAT_SIGNAL:
		return Q_TRAN(uiTemperature);
	}
	return Q_SUPER(uiTop);
}


static QState scrollText(struct UI *me)
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
			return Q_TRAN(uiTemperature);
		}
	}
	return Q_SUPER(scroll);
}


static QState uiRun(struct UI *me)
{
	switch (Q_SIG(me)) {
	case BUTTON_1_PRESS_SIGNAL:
		return Q_TRAN(uiMenuMaybeSettime);
	}
	return Q_SUPER(uiTop);
}


static QState uiPause(struct UI *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		BSP_fast_timers(0, 0);
		QActive_arm((QActive*)me, 1);
		return Q_HANDLED();
	case Q_TIMEOUT_SIG:
		return Q_TRAN(uiTemperature);
	}
	return Q_SUPER(uiRun);
}


static QState uiTemperature(struct UI *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		BSP_fast_timers(1, 0);
		// Start reading the temperature.
		BSP_start_temperature_reading();
		// If we time out, go back to just waiting.
		QActive_armX((QActive*)me, 1, 1);
		return Q_HANDLED();
	case Q_TIMEOUT1_SIG:
		return Q_TRAN(uiGetTemperature);
	}
	return Q_SUPER(uiRun);
}


static QState uiGetTemperature(struct UI *me)
{
	int16_t ti;

	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
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
		return Q_TRAN(uiPause);
	case Q_TIMEOUT1_SIG:
		return Q_TRAN(uiPause);
	}
	return Q_SUPER(uiTemperature);
}



static int16_t get_calibrated_ti(struct UI *me)
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


static void show_temperature(struct UI *me)
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


static void show_temperature_cal(struct UI *me)
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


/** To be used when the user does something in the UI, to reset the timeout
    period. */
#define ACTION() QActive_post((QActive*)me, UI_ACTION_SIGNAL, 0)


static QState uiMenu(struct UI *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		ACTION();
		BSP_fast_timers(1, 2);
		return Q_HANDLED();
	case UI_ACTION_SIGNAL:
		SERIALSTR("U");
		me->timeoutcounter = 4;
		lcd_timeouts(me->timeoutcounter);
		QActive_armX((QActive*)(me), 2, 50);
		return Q_HANDLED();
	case Q_TIMEOUT2_SIG:
		me->timeoutcounter --;
		lcd_timeouts(me->timeoutcounter);
		if (me->timeoutcounter) {
			QActive_armX((QActive*)(me), 2, 50);
			return Q_HANDLED();
		} else {
			return Q_TRAN(uiTemperature);
		}
	case Q_EXIT_SIG:
		lcd_timeouts(0);
		return Q_HANDLED();
	}
	return Q_SUPER(uiTop);
}


// ----- UI Menu Maybe stuff -----


/* The response of the uiMenuMaybe*() functions to button presses determines
   the order of the menu items.  @todo Would this be better in a table, with a
   history pattern? */


static QState uiMenuMaybeSettime(struct UI *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		lcd_showstring("SETTIME");
		return Q_HANDLED();
	case BUTTON_1_PRESS_SIGNAL:
		ACTION();
		return Q_TRAN(uiMenuSettimeHours);
	case BUTTON_2_PRESS_SIGNAL:
		ACTION();
		return Q_TRAN(uiMenuMaybeCalibrate);
	case BUTTON_3_PRESS_SIGNAL:
		ACTION();
		return Q_TRAN(uiMenuMaybeExit);
	}
	return Q_SUPER(uiMenu);
}


static QState uiMenuMaybeCalibrate(struct UI *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		lcd_showstring("T CAL  ");
		return Q_HANDLED();
	case BUTTON_1_PRESS_SIGNAL:
		ACTION();
		return Q_TRAN(uiMenuCalibrateTemperature);
	case BUTTON_2_PRESS_SIGNAL:
		ACTION();
		return Q_TRAN(uiMenuMaybeExit);
	case BUTTON_3_PRESS_SIGNAL:
		ACTION();
		return Q_TRAN(uiMenuMaybeSettime);
	}
	return Q_SUPER(uiMenu);
}


static QState uiMenuMaybeExit(struct UI *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		lcd_showstring("EXIT   ");
		return Q_HANDLED();
	case BUTTON_1_PRESS_SIGNAL:
		ACTION();
		return Q_TRAN(uiTemperature);
	case BUTTON_2_PRESS_SIGNAL:
		ACTION();
		return Q_TRAN(uiMenuMaybeSettime);
	case BUTTON_3_PRESS_SIGNAL:
		ACTION();
		return Q_TRAN(uiMenuMaybeCalibrate);
	}
	return Q_SUPER(uiMenu);
}


// ----- UI Menu Set Time -----


static void display_set_time(struct UI *me,
			     uint8_t show_hours, uint8_t show_minutes);
static void inc_set_time_hours(struct UI *me);
static void dec_set_time_hours(struct UI *me);
static void inc_set_time_minutes(struct UI *me);
static void dec_set_time_minutes(struct UI *me);
static void display_set_time_confirm(struct UI *me, char yn);


static QState uiMenuSettimeHours(struct UI *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		me->settime_hm = 'H';
		me->settime = *gettimep();
		display_set_time(me, TRUE, TRUE);
		QActive_armX((QActive*)me, 1, 17);
		return Q_HANDLED();
	case Q_TIMEOUT1_SIG:
		return Q_TRAN(uiMenuSettimeHoursFlash);
	case BUTTON_1_PRESS_SIGNAL:
		ACTION();
		return Q_TRAN(uiMenuSettimeMinutes);
	case BUTTON_2_PRESS_SIGNAL:
	case BUTTON_2_LONG_PRESS_SIGNAL:
	case BUTTON_2_REPEAT_SIGNAL:
		ACTION();
		inc_set_time_hours(me);
		display_set_time(me, TRUE, TRUE);
		QActive_armX((QActive*)me, 1, 17);
		return Q_HANDLED();
	case BUTTON_3_PRESS_SIGNAL:
	case BUTTON_3_LONG_PRESS_SIGNAL:
	case BUTTON_3_REPEAT_SIGNAL:
		ACTION();
		dec_set_time_hours(me);
		display_set_time(me, TRUE, TRUE);
		QActive_armX((QActive*)me, 1, 17);
		return Q_HANDLED();
	}
	return Q_SUPER(uiMenu);
}


static QState uiMenuSettimeHoursFlash(struct UI *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		display_set_time(me, FALSE, TRUE);
		QActive_armX((QActive*)me, 1, 8);
		return Q_HANDLED();
	case Q_TIMEOUT1_SIG:
		display_set_time(me, TRUE, TRUE);
		return Q_TRAN(uiMenuSettimeHours);
	case BUTTON_2_PRESS_SIGNAL:
	case BUTTON_2_LONG_PRESS_SIGNAL:
	case BUTTON_2_REPEAT_SIGNAL:
	case BUTTON_3_PRESS_SIGNAL:
	case BUTTON_3_LONG_PRESS_SIGNAL:
	case BUTTON_3_REPEAT_SIGNAL:
		display_set_time(me, TRUE, TRUE);
		/* React to the event, but don't handle it. */
		break;
	case Q_EXIT_SIG:
		QActive_armX((QActive*)me, 1, 17);
		return Q_HANDLED();
	}
	return Q_SUPER(uiMenuSettimeHours);
}


static QState uiMenuSettimeMinutes(struct UI *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		me->settime_hm = 'M';
		display_set_time(me, TRUE, TRUE);
		QActive_armX((QActive*)me, 1, 17);
		return Q_HANDLED();
	case Q_TIMEOUT1_SIG:
		return Q_TRAN(uiMenuSettimeMinutesFlash);
	case BUTTON_1_PRESS_SIGNAL:
		ACTION();
		return Q_TRAN(uiMenuSettimeConfirmYes);
	case BUTTON_2_PRESS_SIGNAL:
	case BUTTON_2_LONG_PRESS_SIGNAL:
	case BUTTON_2_REPEAT_SIGNAL:
		ACTION();
		inc_set_time_minutes(me);
		display_set_time(me, TRUE, TRUE);
		QActive_armX((QActive*)me, 1, 17);
		return Q_HANDLED();
	case BUTTON_3_PRESS_SIGNAL:
	case BUTTON_3_LONG_PRESS_SIGNAL:
	case BUTTON_3_REPEAT_SIGNAL:
		ACTION();
		dec_set_time_minutes(me);
		display_set_time(me, TRUE, TRUE);
		QActive_armX((QActive*)me, 1, 17);
		return Q_HANDLED();
	}
	return Q_SUPER(uiMenu);
}


static QState uiMenuSettimeMinutesFlash(struct UI *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		display_set_time(me, TRUE, FALSE);
		QActive_armX((QActive*)me, 1, 8);
		return Q_HANDLED();
	case Q_TIMEOUT1_SIG:
		display_set_time(me, TRUE, TRUE);
		return Q_TRAN(uiMenuSettimeMinutes);
	case BUTTON_2_PRESS_SIGNAL:
	case BUTTON_2_LONG_PRESS_SIGNAL:
	case BUTTON_2_REPEAT_SIGNAL:
	case BUTTON_3_PRESS_SIGNAL:
	case BUTTON_3_LONG_PRESS_SIGNAL:
	case BUTTON_3_REPEAT_SIGNAL:
		display_set_time(me, TRUE, TRUE);
		/* React to the event, but don't handle it. */
		break;
	case Q_EXIT_SIG:
		QActive_armX((QActive*)me, 1, 17);
		return Q_HANDLED();
	}
	return Q_SUPER(uiMenuSettimeMinutes);
}


static QState uiMenuSettimeConfirmYes(struct UI *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		display_set_time_confirm(me, 'Y');
		return Q_HANDLED();
	case BUTTON_1_PRESS_SIGNAL:
		ACTION();
		me->settime.seconds = 0;
		*gettimep() = me->settime;
		return Q_TRAN(uiMenuMaybeSettime);
	case BUTTON_2_PRESS_SIGNAL:
	case BUTTON_3_PRESS_SIGNAL:
		ACTION();
		return Q_TRAN(uiMenuSettimeConfirmNo);
		return Q_HANDLED();
	}
	return Q_SUPER(uiMenu);
}


static QState uiMenuSettimeConfirmNo(struct UI *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		display_set_time_confirm(me, 'N');
		return Q_HANDLED();
	case BUTTON_1_PRESS_SIGNAL:
		ACTION();
		return Q_TRAN(uiMenuMaybeSettime);
	case BUTTON_2_PRESS_SIGNAL:
	case BUTTON_3_PRESS_SIGNAL:
		ACTION();
		return Q_TRAN(uiMenuSettimeConfirmYes);
		return Q_HANDLED();
	}
	return Q_SUPER(uiMenu);
}


static void display_set_time(struct UI *me,
			     uint8_t show_hours, uint8_t show_minutes)
{
	char disp[8];

	disp[0] = me->settime_hm;
	disp[1] = disp[2] = ' ';
	if (show_hours) {
		disp[3] = me->settime.ht;
		disp[4] = me->settime.h1;
	} else {
		disp[3] = disp[4] = ' ';
	}
	if (show_minutes) {
		disp[5] = me->settime.mt;
		disp[6] = me->settime.m1;
	} else {
		disp[5] = disp[6] = ' ';
	}
	disp[7] = '\0';
	lcd_showstring(disp);
}


static void inc_set_time_hours(struct UI *me)
{
	struct Time *time;

	time = &me->settime;
	if (time->ht == '2' && time->h1 == '3') {
		time->ht = '0';
		time->h1 = '0';
	} else if (time->h1 == '9') {
		time->h1 = '0';
		time->ht ++;
	} else {
		time->h1 ++;
	}
}


static void dec_set_time_hours(struct UI *me)
{
	struct Time *time;

	time = &me->settime;
	if (time->ht == '0' && time->h1 == '0') {
		time->ht = '2';
		time->h1 = '3';
	} else if (time->h1 == '0') {
		time->h1 = '9';
		time->ht --;
	} else {
		time->h1 --;
	}
}


static void inc_set_time_minutes(struct UI *me)
{
	struct Time *time;

	time = &me->settime;
	if (time->mt == '5' && time->m1 == '9') {
		time->mt = '0';
		time->m1 = '0';
	} else if (time->m1 == '9') {
		time->m1 = '0';
		time->mt ++;
	} else {
		time->m1 ++;
	}
}


static void dec_set_time_minutes(struct UI *me)
{
	struct Time *time;

	time = &me->settime;
	if (time->mt == '0' && time->m1 == '0') {
		time->mt = '5';
		time->m1 = '9';
	} else if (time->m1 == '0') {
		time->m1 = '9';
		time->mt --;
	} else {
		time->m1 --;
	}
}


static void display_set_time_confirm(struct UI *me, char yn)
{
	char disp[8];

	disp[0] = me->settime.ht;
	disp[1] = me->settime.h1;
	disp[2] = me->settime.mt;
	disp[3] = me->settime.m1;
	disp[4] = ' ';
	disp[5] = ' ';
	disp[6] = yn;
	disp[7] = '\0';
	lcd_showstring(disp);
}


// ----- UI Menu Calibrate -----


static QState uiMenuCalibrate(struct UI *me)
{
	switch (Q_SIG(me)) {
	case BUTTON_1_PRESS_SIGNAL:
		ACTION();
		SERIALSTR("b1\r\n");
		BSP_save_calibration(me->calibration);
		return Q_TRAN(uiMenuMaybeCalibrate);
	case BUTTON_2_PRESS_SIGNAL:
		ACTION();
		if (me->calibration < MAX_CAL) {
			SERIALSTR("up\r\n");
			me->calibration ++;
			show_temperature_cal(me);
		}
		return Q_HANDLED();
	case BUTTON_3_PRESS_SIGNAL:
		ACTION();
		if (me->calibration > MIN_CAL) {
			SERIALSTR("down\r\n");
			me->calibration --;
			show_temperature_cal(me);
		}
		return Q_HANDLED();
	}
	return Q_SUPER(uiMenu);
}


static QState uiMenuCalibratePause(struct UI *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		QActive_armX((QActive*)me, 1, 28);
		return Q_HANDLED();
	case Q_TIMEOUT1_SIG:
		return Q_TRAN(uiMenuCalibrateTemperature);
	}
	return Q_SUPER(uiMenuCalibrate);
}


static QState uiMenuCalibrateTemperature(struct UI *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		SERIALSTR("uiCP\r\n");
		show_temperature_cal(me);
		BSP_start_temperature_reading();
		/* Need two ticks to guarantee that we have at least one full
		   tick. */
		QActive_armX((QActive*)me, 1, 2);
		return Q_HANDLED();
	case Q_TIMEOUT1_SIG:
		return Q_TRAN(uiMenuCalibrateGetTemperature);
	case Q_EXIT_SIG:
		/* Save this as an invalid value, so at the next tick
		   uiGetTemperature() will be forced to update the display. */
		me->ti = INVALIDTI;
		return Q_HANDLED();
	}
	return Q_SUPER(uiMenuCalibrate);
}


static QState uiMenuCalibrateGetTemperature(struct UI *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		SERIALSTR("uiMCT\r\n");
		BSP_get_temperature();
		/* Only need one tick to get the temperature, since we're
		   guaranteed to be arriving here very near the start of a
		   tick period. */
		QActive_armX((QActive*)me, 1, 1);
		return Q_HANDLED();
	case TEMPERATURE_SIGNAL:
		me->ti = Q_PAR(me);
		show_temperature_cal(me);
		return Q_TRAN(uiMenuCalibrateTemperature);
	case Q_TIMEOUT1_SIG:
		/* Nothing has happened - give up. */
		return Q_TRAN(uiMenuCalibratePause);
	}
	return Q_SUPER(uiMenuCalibrate);
}