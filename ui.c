#include "ui.h"
#include "hc.h"
#include "bsp.h"
#include "serial.h"
#include "lcd.h"
#include "rtc.h"
#include "buttons.h"
#include "time.h"
#include "titime.h"
#include "recorder.h"
#include <string.h>
#include <stdio.h>		/* For snprintf() */
/* Contains declarations of temperature sensing ranges: MINTI, MAXTI,
   NCONVERSIONS. */
#include "bsp-temperature-scale.h"

#include "truefalse.h"


Q_DEFINE_THIS_MODULE("u");


static QState uiInitial(struct UI *me);
static QState uiTop(struct UI *me);

static QState scroll(struct UI *me);
static QState scrollText(struct UI *me);

static QState uiRun(struct UI *me);

static QState uiMenu(struct UI *me);

static QState uiShowMaxOrMin(struct UI *me);
static QState uiShowMaxTop(struct UI *me);
static QState uiShowMax(struct UI *me);
static QState uiShowMinTop(struct UI *me);
static QState uiShowMin(struct UI *me);

static QState uiMenuMaybeSettime(struct UI *me);
static QState uiMenuSettimeYears(struct UI *me);
static QState uiMenuSettimeYearsFlash(struct UI *me);
static QState uiMenuSettimeMonths(struct UI *me);
static QState uiMenuSettimeMonthsFlash(struct UI *me);
static QState uiMenuSettimeDaysCheckmax(struct UI *me);
static QState uiMenuSettimeDays(struct UI *me);
static QState uiMenuSettimeDaysFlash(struct UI *me);
static QState uiMenuSettimeHours(struct UI *me);
static QState uiMenuSettimeHoursFlash(struct UI *me);
static QState uiMenuSettimeMinutes(struct UI *me);
static QState uiMenuSettimeMinutesFlash(struct UI *me);

static QState uiMenuMaybeCalibrate(struct UI *me);
static QState uiMenuCalibratePause(struct UI *me);
static QState uiMenuCalibrateTemperature(struct UI *me);
static QState uiMenuCalibrateGetTemperature(struct UI *me);

static QState uiMenuMaybeAdjusttime(struct UI *me);
static QState uiMenuAdjusttime(struct UI *me);

static void show_temperature(int16_t t);
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
	ui.cal = get_calibration();
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
		/** Hack alert!  This code rashly assumes that the ht, h1, mt,
		    and m1 fields contain ASCII digits, and that they are
		    consecutive in memory.  This code broke when I added the
		    year, month, and day fields to struct Time, because it
		    previously assumed that &time was the same as
		    &(time->ht). */
		lcd_showdigits((const char *)(&(time->ht)));
		if (time->seconds & 0x02) {
			lcd_colon(1);
		} else {
			lcd_colon(0);
		}
		return Q_HANDLED();
	case CURRENT_TEMPERATURE_SIGNAL:
		/* Handle this signal here, so that no matter where the UI is,
		   the current temperature will be available when it's
		   finished. */
		me->ti = (int16_t) Q_PAR(me);
		return Q_HANDLED();
	case BUTTON_CANCEL_PRESS_SIGNAL:
		return Q_TRAN(uiRun);
	}
	return Q_SUPER(QHsm_top);
}


static QState scroll(struct UI *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		SERIALSTR("scroll");
		me->scrollstring = banner;
		me->scrollindex = 0;
		BSP_fast_timer_1(TRUE);
		return Q_HANDLED();
	case BUTTON_ENTER_PRESS_SIGNAL:
	case BUTTON_ENTER_LONG_PRESS_SIGNAL:
	case BUTTON_ENTER_REPEAT_SIGNAL:
	case BUTTON_UP_PRESS_SIGNAL:
	case BUTTON_UP_LONG_PRESS_SIGNAL:
	case BUTTON_UP_REPEAT_SIGNAL:
	case BUTTON_DOWN_PRESS_SIGNAL:
	case BUTTON_DOWN_LONG_PRESS_SIGNAL:
	case BUTTON_DOWN_REPEAT_SIGNAL:
	case BUTTON_CANCEL_PRESS_SIGNAL:
	case BUTTON_CANCEL_LONG_PRESS_SIGNAL:
	case BUTTON_CANCEL_REPEAT_SIGNAL:
		return Q_TRAN(uiRun);
	case Q_EXIT_SIG:
		BSP_fast_timer_1(FALSE);
		/* If we exit here on a button press, the buttons AO (or the
		   Basic Timer 1 ISR) may see the button down immediately, with
		   the result that the UI immediately enters the menu.  Avoid
		   that by telling the buttons to wait. */
		post(&buttons.super, BUTTONS_WAIT_SIGNAL, 0);
		return Q_HANDLED();
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
			return Q_TRAN(uiRun);
		}
	}
	return Q_SUPER(scroll);
}


static QState uiRun(struct UI *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		show_temperature(me->ti);
		return Q_HANDLED();
	case BUTTON_ENTER_PRESS_SIGNAL:
		return Q_TRAN(uiMenuMaybeSettime);
	case BUTTON_UP_PRESS_SIGNAL:
		return Q_TRAN(uiShowMax);
	case BUTTON_DOWN_PRESS_SIGNAL:
		return Q_TRAN(uiShowMin);
	case CURRENT_TEMPERATURE_SIGNAL:
		me->ti = (int16_t) Q_PAR(me);
		show_temperature(me->ti);
	}
	return Q_SUPER(uiTop);
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
		adjti = me->ti + me->cal;
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


static void show_temperature(int16_t t)
{
	const char *tstring;

	SERIALSTR("t: ");
	serial_send_int(t);
	if (INVALIDTI == t) {
		tstring = "  ???  ";
	} else if (LOWTI == t) {
		tstring = "  ?" "?-  ";
	} else if (MINTI == t) {
		tstring = "  ?--  ";
	} else if (HIGHTI == t) {
		tstring = "  ?" "?+  ";
	} else if (MAXTI == t) {
		tstring = "  ?++  ";
	} else {
		SERIALSTR(":");
		t -= MINTI;	    /* Move the scale up to zero-based. */
		Q_ASSERT( t >= 0 ); /* Range checking. */
		Q_ASSERT( t < NCONVERSIONS );
		t /= 2;		/* Scale to whole degrees. */
		serial_send_int(t);
		tstring = tstrings[t];
	}
	SERIALSTR("\"");
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

	Q_ASSERT( me->cal >= MIN_CAL );
	Q_ASSERT( me->cal <= MAX_CAL );

	strcpy(s+4, calstrings[ me->cal - MIN_CAL ]);

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


// ----- max and min -----


static void show_at(const struct Time *at)
{
	char buf[8];

	buf[7] = 'a';
	snprintf(buf, 8, "AT %c%c%c%c", at->ht, at->h1, at->mt, at->m1);
	Q_ASSERT( ! buf[7] );
	SERIALSTR("AT(");
	serial_send_int(at->ht);
	serial_send_char(':');
	serial_send_int(at->h1);
	serial_send_char(':');
	serial_send_int(at->mt);
	serial_send_char(':');
	serial_send_int(at->m1);
	SERIALSTR(")");
	lcd_showstring(buf);
}


static QState uiShowMaxOrMin(struct UI *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		BSP_fast_timer_1(TRUE);
		lcd_showdigits("    ");
		lcd_colon(0);
		return Q_HANDLED();
	case TIME_SIGNAL:
		/* Ignore time updates so we don't display anything on the time
		   digits while we're displaying a max or min. */
		return Q_HANDLED();
	case BUTTON_CANCEL_PRESS_SIGNAL:
		return Q_TRAN(uiRun);
	case Q_EXIT_SIG:
		BSP_fast_timer_1(FALSE);
		return Q_HANDLED();
	}
	return Q_SUPER(uiTop);
}


static const QTimeEvtCtr showmaxmintimeout = 15;


static QState uiShowMaxTop(struct UI *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		me->showmaxmincounter = 0;
		return Q_HANDLED();
	case BUTTON_DOWN_PRESS_SIGNAL:
		return Q_TRAN(uiShowMin);
	}
	return Q_SUPER(uiShowMaxOrMin);
}


static QState uiShowMax(struct UI *me)
{
	QTimeEvtCtr timeout = showmaxmintimeout;
	int16_t ti;

	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		me->showmaxmincounter ++;
		switch (me->showmaxmincounter) {
		case 1:
			lcd_showstring("MAX T  ");
			timeout = showmaxmintimeout;
			break;
		case 2:
			lcd_showstring("SINCE  ");
			timeout = showmaxmintimeout;
			break;
		case 3:
			lcd_showstring("3 PM   ");
			timeout = showmaxmintimeout;
			break;
		case 4:
			ti = get_max_today()->ti;
			if (INVALIDTI == ti)
				me->showmaxmincounter ++;
			show_temperature(ti);
			timeout = 3 * showmaxmintimeout;
			break;
		case 5:
			show_at(&(get_max_today()->time));
			timeout = 3 * showmaxmintimeout;
			break;
		case 6:
			lcd_showstring("MAX T  ");
			timeout = showmaxmintimeout;
			break;
		case 7:
			lcd_showstring("TO LAST");
			timeout = showmaxmintimeout;
			break;
		case 8:
			lcd_showstring("3 PM   ");
			timeout = showmaxmintimeout;
			break;
		case 9:
			ti = get_max_yesterday()->ti;
			if (INVALIDTI == ti)
				me->showmaxmincounter ++;
			show_temperature(ti);
			timeout = 3 * showmaxmintimeout;
			break;
		case 10:
			show_at(&(get_max_yesterday()->time));
			timeout = 3 * showmaxmintimeout;
			break;
		default:
			Q_ASSERT( 0 );
			break;
		}
		QActive_armX(&me->super, 1, timeout);
		return Q_HANDLED();
	case Q_TIMEOUT1_SIG:
	case BUTTON_UP_PRESS_SIGNAL:
		// This number must match the highest number above.
		if (10 == me->showmaxmincounter)
			return Q_TRAN(uiRun);
		else
			return Q_TRAN(uiShowMax);
	}
	return Q_SUPER(uiShowMaxTop);
}


static QState uiShowMinTop(struct UI *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		me->showmaxmincounter = 0;
		return Q_HANDLED();
	case BUTTON_UP_PRESS_SIGNAL:
		return Q_TRAN(uiShowMax);
	}
	return Q_SUPER(uiShowMaxOrMin);
}


static QState uiShowMin(struct UI *me)
{
	QTimeEvtCtr timeout = showmaxmintimeout;
	int16_t ti;

	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		me->showmaxmincounter ++;
		switch (me->showmaxmincounter) {
		case 1:
			lcd_showstring("MIN T  ");
			timeout = showmaxmintimeout;
			break;
		case 2:
			lcd_showstring("SINCE  ");
			timeout = showmaxmintimeout;
			break;
		case 3:
			lcd_showstring("9 AM   ");
			timeout = showmaxmintimeout;
			break;
		case 4:
			ti = get_min_today()->ti;
			if (INVALIDTI == ti)
				me->showmaxmincounter ++;
			show_temperature(ti);
			timeout = 3 * showmaxmintimeout;
			break;
		case 5:
			show_at(&(get_min_today()->time));
			timeout = 3 * showmaxmintimeout;
			break;
		case 6:
			lcd_showstring("MIN T  ");
			timeout = showmaxmintimeout;
			break;
		case 7:
			lcd_showstring("TO LAST");
			timeout = showmaxmintimeout;
			break;
		case 8:
			lcd_showstring("9 AM   ");
			timeout = showmaxmintimeout;
			break;
		case 9:
			ti = get_min_yesterday()->ti;
			if (INVALIDTI == ti)
				me->showmaxmincounter ++;
			show_temperature(ti);
			timeout = 3 * showmaxmintimeout;
			break;
		case 10:
			show_at(&(get_min_yesterday()->time));
			timeout = 3 * showmaxmintimeout;
			break;
		default:
			Q_ASSERT( 0 );
			break;
		}
		QActive_armX(&me->super, 1, timeout);
		return Q_HANDLED();
	case Q_TIMEOUT1_SIG:
	case BUTTON_DOWN_PRESS_SIGNAL:
		if (10 == me->showmaxmincounter)
			return Q_TRAN(uiRun);
		else
			return Q_TRAN(uiShowMin);
	}
	return Q_SUPER(uiShowMinTop);
}


// ----- ui menu -----


/** To be used when the user does something in the UI, to reset the timeout
    period. */
#define ACTION() QActive_post((QActive*)me, UI_ACTION_SIGNAL, 0)


static QState uiMenu(struct UI *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		ACTION();
		BSP_fast_timer_1(TRUE);
		BSP_fast_timer_2(TRUE);
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
			return Q_TRAN(uiRun);
		}
	case BUTTON_CANCEL_PRESS_SIGNAL:
		return Q_TRAN(uiRun);
	case Q_EXIT_SIG:
		BSP_fast_timer_1(FALSE);
		BSP_fast_timer_2(FALSE);
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
	case BUTTON_ENTER_PRESS_SIGNAL:
		ACTION();
		me->settime = *gettimep();
		return Q_TRAN(uiMenuSettimeYears);
	case BUTTON_UP_PRESS_SIGNAL:
		ACTION();
		return Q_TRAN(uiMenuMaybeCalibrate);
	case BUTTON_DOWN_PRESS_SIGNAL:
		ACTION();
		return Q_TRAN(uiMenuMaybeAdjusttime);
	}
	return Q_SUPER(uiMenu);
}


static QState uiMenuMaybeCalibrate(struct UI *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		lcd_showstring("CALTEMP");
		return Q_HANDLED();
	case BUTTON_ENTER_PRESS_SIGNAL:
		ACTION();
		return Q_TRAN(uiMenuCalibrateTemperature);
	case BUTTON_UP_PRESS_SIGNAL:
		ACTION();
		return Q_TRAN(uiMenuMaybeAdjusttime);
	case BUTTON_DOWN_PRESS_SIGNAL:
		ACTION();
		return Q_TRAN(uiMenuMaybeSettime);
	}
	return Q_SUPER(uiMenu);
}


static QState uiMenuMaybeAdjusttime(struct UI *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		lcd_showstring("ADJTIME");
		return Q_HANDLED();
	case BUTTON_ENTER_PRESS_SIGNAL:
		ACTION();
		return Q_TRAN(uiMenuAdjusttime);
	case BUTTON_UP_PRESS_SIGNAL:
		ACTION();
		return Q_TRAN(uiMenuMaybeSettime);
	case BUTTON_DOWN_PRESS_SIGNAL:
		ACTION();
		return Q_TRAN(uiMenuMaybeCalibrate);
	}
	return Q_SUPER(uiMenu);
}


// ----- UI Menu Set Time -----


static void display_set_year(struct UI *me, uint8_t on);
static void display_set_month(struct UI *me, uint8_t on);
static void display_set_day(struct UI *me, uint8_t on);
static void display_set_time(struct UI *me,
			     uint8_t show_hours, uint8_t show_minutes);
static inline void display_set_hour(struct UI *me, uint8_t on) {
	display_set_time(me, on, TRUE);
}

static inline void display_set_minute(struct UI *me, uint8_t on) {
	display_set_time(me, TRUE, on);
}


static inline void inc_settime_year(struct UI *me) {
	inc_year(&me->settime);
}

static inline void dec_settime_year(struct UI *me) {
	dec_year(&me->settime);
}

static inline void inc_settime_month(struct UI *me) {
	inc_month(&me->settime);
}

static inline void dec_settime_month(struct UI *me) {
	dec_month(&me->settime);
}

static inline void inc_settime_day(struct UI *me) {
	inc_day(&me->settime);
}

static inline void dec_settime_day(struct UI *me) {
	dec_day(&me->settime);
}


#define M_CHECK()

/**
 * Create two functions to manage setting one time element.
 *
 * @param name_ The name of this time setting state
 *
 * @param flash_ The name of the state that flashes the time element we're
 * setting.
 *
 * @param parent_ The name of the parent state to the first of the time setting
 * states.  Would normally be uiMenu, but the days setting requires some
 * specialised entry code.
 *
 * @param next_ The name of the state to go to when the user accepts the current
 * selection.
 *
 * @param displaych_ The character to display at the left end of the LCD,
 * indicating what we are setting.
 *
 * @param display_ A function to display the current value on the LCD.  It gets
 * called with two args - a pointer to struct Time, and a boolean value to
 * indicate whether we're in the on or off part of the flash.
 *
 * @param inc_ A function to increase the current value.  It gets called with
 * one arg, a pointer to struct Time.
 *
 * @param dec_ A function to decrease the current value.  It gets called with
 * one arg, a pointer to struct Time.
 */
#define M(name_,flash_,parent_,next_,displaych_,display_,inc_,dec_)	\
	static QState name_(struct UI *me)				\
	{								\
		switch (Q_SIG(me)) {					\
		case Q_ENTRY_SIG:					\
			me->settime_YmdHM = displaych_;			\
			/* Include code here to check the initial */	\
			/* conditions for this time setting state. */	\
			M_CHECK();					\
			display_(me, TRUE);				\
			QActive_armX((QActive*)me, 1, 17);		\
			return Q_HANDLED();				\
		case Q_TIMEOUT1_SIG:					\
		return Q_TRAN(flash_);					\
		case BUTTON_ENTER_PRESS_SIGNAL:				\
			ACTION();					\
			return Q_TRAN(next_);				\
		case BUTTON_UP_PRESS_SIGNAL:				\
		case BUTTON_UP_LONG_PRESS_SIGNAL:			\
		case BUTTON_UP_REPEAT_SIGNAL:				\
			ACTION();					\
			inc_(me);					\
			display_(me, TRUE);				\
			QActive_armX((QActive*)me, 1, 17);		\
			return Q_HANDLED();				\
		case BUTTON_DOWN_PRESS_SIGNAL:				\
		case BUTTON_DOWN_LONG_PRESS_SIGNAL:			\
		case BUTTON_DOWN_REPEAT_SIGNAL:				\
			ACTION();					\
			dec_(me);					\
			display_(me, TRUE);				\
			QActive_armX((QActive*)me, 1, 17);		\
			return Q_HANDLED();				\
		}							\
		return Q_SUPER(parent_);				\
	}								\
									\
	static QState flash_(struct UI *me)				\
	{								\
		switch (Q_SIG(me)) {					\
		case Q_ENTRY_SIG:					\
			display_(me, FALSE);				\
			QActive_armX((QActive*)me, 1, 8);		\
			return Q_HANDLED();				\
		case Q_TIMEOUT1_SIG:					\
			display_(me, TRUE);				\
			return Q_TRAN(name_);				\
		case BUTTON_DOWN_PRESS_SIGNAL:				\
		case BUTTON_DOWN_LONG_PRESS_SIGNAL:			\
		case BUTTON_DOWN_REPEAT_SIGNAL:				\
		case BUTTON_UP_PRESS_SIGNAL:				\
		case BUTTON_UP_LONG_PRESS_SIGNAL:			\
		case BUTTON_UP_REPEAT_SIGNAL:				\
			display_(me, TRUE);				\
			/* React to the event, but don't handle it. */	\
			break;						\
		case Q_EXIT_SIG:					\
			QActive_armX((QActive*)me, 1, 17);		\
			return Q_HANDLED();				\
		}							\
		return Q_SUPER(name_);					\
	}


M(uiMenuSettimeYears, uiMenuSettimeYearsFlash, uiMenu, uiMenuSettimeMonths,
  'Y', display_set_year, inc_settime_year, dec_settime_year);

M(uiMenuSettimeMonths, uiMenuSettimeMonthsFlash, uiMenu, uiMenuSettimeDays,
  'm', display_set_month, inc_settime_month, dec_settime_month);

#undef M_CHECK

/**
 * This macro is defined before the M() macro call for the days setting state.
 * It checks that the number of days does not exceed the maximum number of days
 * for the set month, so we don't try to set Feb 31st, for example.
 */
#define M_CHECK()							\
	{								\
	uint8_t max_day = max_day_for_month(&me->settime);		\
	if (me->settime.day > max_day) {				\
		me->settime.day = max_day;				\
	}								\
	}

M(uiMenuSettimeDays, uiMenuSettimeDaysFlash, uiMenuSettimeDaysCheckmax,
  uiMenuSettimeHours,
  'd', display_set_day, inc_settime_day, dec_settime_day);

#undef M_CHECK

/**
 * Provides an extra entry action for uiMenuSettimeDays().
 *
 * Because the code for uiMenuSettimeDays() is generated by a macro, we can't
 * add extra code in its entry action to check the range of the days of the
 * month.  So add an extra state instead.  The only thing that this state does
 * is to check the days range on entry.
 */
static QState uiMenuSettimeDaysCheckmax(struct UI *me)
{
	uint8_t max_day;

	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		max_day = max_day_for_month(&me->settime);
		if (me->settime.day > max_day) {
			me->settime.day = max_day;
		}
		return Q_HANDLED();
	}
	return Q_SUPER(uiMenu);
}


/**
 * @TODO Implement all the functions below with the M() macro.  Make sure the
 * new ones (year, month, day) work before replacing the existing ones.
 *
 * @TODO make the names consistent.  There's a mix of camel case and _
 * separated names in here.  Making that consistent will allow us to use token
 * pasting and simplify the macro calls.
 *
 * @todo use token pasting (ie a##b)
 */


static QState uiMenuSettimeHours(struct UI *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		me->settime_YmdHM = 'H';
		display_set_time(me, TRUE, TRUE);
		QActive_armX((QActive*)me, 1, 17);
		return Q_HANDLED();
	case Q_TIMEOUT1_SIG:
		return Q_TRAN(uiMenuSettimeHoursFlash);
	case BUTTON_ENTER_PRESS_SIGNAL:
		ACTION();
		return Q_TRAN(uiMenuSettimeMinutes);
	case BUTTON_UP_PRESS_SIGNAL:
	case BUTTON_UP_LONG_PRESS_SIGNAL:
	case BUTTON_UP_REPEAT_SIGNAL:
		ACTION();
		inc_hour(&me->settime);
		display_set_time(me, TRUE, TRUE);
		QActive_armX((QActive*)me, 1, 17);
		return Q_HANDLED();
	case BUTTON_DOWN_PRESS_SIGNAL:
	case BUTTON_DOWN_LONG_PRESS_SIGNAL:
	case BUTTON_DOWN_REPEAT_SIGNAL:
		ACTION();
		dec_hour(&me->settime);
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
	case BUTTON_UP_PRESS_SIGNAL:
	case BUTTON_UP_LONG_PRESS_SIGNAL:
	case BUTTON_UP_REPEAT_SIGNAL:
	case BUTTON_DOWN_PRESS_SIGNAL:
	case BUTTON_DOWN_LONG_PRESS_SIGNAL:
	case BUTTON_DOWN_REPEAT_SIGNAL:
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
		me->settime_YmdHM = 'M';
		display_set_time(me, TRUE, TRUE);
		QActive_armX((QActive*)me, 1, 17);
		return Q_HANDLED();
	case Q_TIMEOUT1_SIG:
		return Q_TRAN(uiMenuSettimeMinutesFlash);
	case BUTTON_ENTER_PRESS_SIGNAL:
		ACTION();
		me->settime.seconds = 0;
		set_rtc_time(&me->settime);
		return Q_TRAN(uiMenuMaybeSettime);
	case BUTTON_UP_PRESS_SIGNAL:
	case BUTTON_UP_LONG_PRESS_SIGNAL:
	case BUTTON_UP_REPEAT_SIGNAL:
		ACTION();
		inc_minute(&me->settime);
		display_set_time(me, TRUE, TRUE);
		QActive_armX((QActive*)me, 1, 17);
		return Q_HANDLED();
	case BUTTON_DOWN_PRESS_SIGNAL:
	case BUTTON_DOWN_LONG_PRESS_SIGNAL:
	case BUTTON_DOWN_REPEAT_SIGNAL:
		ACTION();
		dec_minute(&me->settime);
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
	case BUTTON_UP_PRESS_SIGNAL:
	case BUTTON_UP_LONG_PRESS_SIGNAL:
	case BUTTON_UP_REPEAT_SIGNAL:
	case BUTTON_DOWN_PRESS_SIGNAL:
	case BUTTON_DOWN_LONG_PRESS_SIGNAL:
	case BUTTON_DOWN_REPEAT_SIGNAL:
		display_set_time(me, TRUE, TRUE);
		/* React to the event, but don't handle it. */
		break;
	case Q_EXIT_SIG:
		QActive_armX((QActive*)me, 1, 17);
		return Q_HANDLED();
	}
	return Q_SUPER(uiMenuSettimeMinutes);
}


static void display_set_year(struct UI *me, uint8_t on)
{
	char disp[8];

	SERIALSTR("<dsY>");

	Q_ASSERT( me->settime.year >= MIN_YEAR );
	Q_ASSERT( me->settime.year <= MAX_YEAR );
	disp[0] = me->settime_YmdHM;
	if (on) {
		disp[1] = disp[2] = ' ';
		snprintf(disp+3, 5, "%4d", me->settime.year);
	} else {
		disp[1] = disp[2] = disp[3] = disp[4] = disp[5] = disp[6] = ' ';
	}
	disp[7] = '\0';
	SERIALSTR("{");
	serial_send(disp);
	SERIALSTR("}");
	lcd_showstring(disp);
}


static void display_set_month(struct UI *me, uint8_t on)
{
	char disp[8];
	const char *mname = "???";

	SERIALSTR("<dsm>");

	Q_ASSERT( me->settime.month <= 12 );
	disp[0] = me->settime_YmdHM;
	if (on) {
		switch (me->settime.month) {
		case 1: mname = "JAN"; break;
		case 2: mname = "FEB"; break;
		case 3: mname = "MAR"; break;
		case 4: mname = "APR"; break;
		case 5: mname = "MAY"; break;
		case 6: mname = "JUN"; break;
		case 7: mname = "JUL"; break;
		case 8: mname = "AUG"; break;
		case 9: mname = "SEP"; break;
		case 10: mname = "OCT"; break;
		case 11: mname = "NOV"; break;
		case 12: mname = "DEC"; break;
		}
		disp[1] = disp[2] = disp[3] = ' ';
		strncpy(disp+4, mname, 4);
		Q_ASSERT( ! disp[7] );
	} else {
		disp[1] = disp[2] = disp[3] = ' ';
		disp[4] = disp[5] = disp[6] = ' ';
		disp[7] = '\0';
	}
	lcd_showstring(disp);
}


static void display_set_day(struct UI *me, uint8_t on)
{
	char disp[8];

	SERIALSTR("<dsd>");

	snprintf(disp, 8, "Day  %2u", me->settime.day);
	Q_ASSERT( ! disp[7] );
	lcd_showstring(disp);
}


static void display_set_time(struct UI *me,
			     uint8_t show_hours, uint8_t show_minutes)
{
	char disp[8];

	SERIALSTR("<dsT>");

	snprintf(disp, 8, "%c  %c%c%c%c", me->settime_YmdHM,
		 show_hours ? me->settime.ht : ' ',
		 show_hours ? me->settime.h1 : ' ',
		 show_minutes ? me->settime.mt : ' ',
		 show_minutes ? me->settime.m1 : ' ');
	Q_ASSERT( ! disp[7] );
	SERIALSTR("{");
	serial_send(disp);
	SERIALSTR("}");
	lcd_showstring(disp);
}


// ----- UI Menu Calibrate -----


static QState uiMenuCalibrate(struct UI *me)
{
	switch (Q_SIG(me)) {
	case BUTTON_ENTER_PRESS_SIGNAL:
		ACTION();
		SERIALSTR("b1\r\n");
		set_calibration(me->cal);
		return Q_TRAN(uiMenuMaybeCalibrate);
	case BUTTON_UP_PRESS_SIGNAL:
		ACTION();
		if (me->cal < MAX_CAL) {
			SERIALSTR("up\r\n");
			me->cal ++;
			show_temperature_cal(me);
		}
		return Q_HANDLED();
	case BUTTON_DOWN_PRESS_SIGNAL:
		ACTION();
		if (me->cal > MIN_CAL) {
			SERIALSTR("down\r\n");
			me->cal --;
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


#include "ui-rtc-adj.inc"


static void display_adjusttime(struct UI *me)
{
	int16_t adj = me->adjustment;

	Q_ASSERT( adj <= MAX_ADJ );
	Q_ASSERT( adj >= MIN_ADJ );
	adj -= MIN_ADJ;
	SERIALSTR("adjust:\"");
	serial_send(adjuststrings[adj]);
	SERIALSTR("\"");
	lcd_showstring(adjuststrings[adj]);
}

static QState uiMenuAdjusttime(struct UI *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		SERIALSTR("uiMAT\r\n");
		me->adjustment = BSP_get_adjustment();
		display_adjusttime(me);
		return Q_HANDLED();
	case BUTTON_ENTER_PRESS_SIGNAL:
		ACTION();
		SERIALSTR("b1\r\n");
		BSP_save_adjustment(me->adjustment);
		return Q_TRAN(uiMenuMaybeAdjusttime);
	case BUTTON_UP_PRESS_SIGNAL:
		ACTION();
		if (me->adjustment < MAX_ADJ) {
			SERIALSTR("up\r\n");
			me->adjustment ++;
			display_adjusttime(me);
		}
		return Q_HANDLED();
	case BUTTON_DOWN_PRESS_SIGNAL:
		ACTION();
		if (me->adjustment > MIN_ADJ) {
			SERIALSTR("down\r\n");
			me->adjustment --;
			display_adjusttime(me);
		}
		return Q_HANDLED();
	}
	return Q_SUPER(uiMenu);
}
