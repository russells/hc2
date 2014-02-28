#include "rtc.h"
#include "ui.h"
#include "lcd.h"
#include "bsp.h"
#include "serial.h"
#include <string.h>
#include <stdlib.h>

/**
 * @file
 *
 * Implement our real time clock.
 *
 * Currently this never does any state transitions (except for the initial
 * transition.)  That capability is more of a place holder for when we
 * implement time setting and perhaps other things in the clock.
 */

Q_DEFINE_THIS_MODULE("r");


static QState initial(struct RTC *me);
static QState counting(struct RTC *me);


struct RTC rtc;


struct TimeOnly {
	char ht;
	char h1;
	char mt;
	char m1;
};


#include "rtc-add-sub.inc"


void rtc_ctor(void)
{
	QActive_ctor((QActive*)(&rtc), (QStateHandler)(&initial));
	rtc.time.year = MIN_YEAR;
	rtc.time.month = 3;
	rtc.time.day = 7;
	rtc.time.ht = '1';
	rtc.time.h1 = '7';
	rtc.time.mt = '3';
	rtc.time.m1 = '0';
	rtc.time.seconds = 0;
}


void set_rtc_time(const struct Time *time)
{
	rtc.time = *time;
}


static QState initial(struct RTC *me)
{
	return Q_TRAN(counting);
}


//static int compare_timeonly(const struct TimeOnly *a, const struct TimeOnly *b)
static int compare_timeonly(const void *a, const void *b)
{
	SERIALSTR(".");
	return strncmp((const char *)a, (const char *)b, 4);
}


static void do_adjustment(struct RTC *me)
{
	struct TimeOnly *now;
	int16_t adj;

	adj = BSP_get_adjustment();

	if (adj) {
		int16_t absadj = adj > 0 ? adj : -adj;
		/* Find out if the current minute is subject to adjustment.  We
		   make the first second of that minute 1/8 second longer or
		   shorter. */
		now = bsearch(
			      /* key */
			      (const struct TimeOnly *)(&(me->time.ht)),
			      /* base (array to search) */
			      adjustments[absadj],
			      /* number of elements - twice because we adjust
				 in 1/8 seconds, but the UI is in 1/4
				 seconds. */
			      absadj*2,
			      /* We only compare hours and minutes */
			      4,
			      /* compare function */
			      compare_timeonly);
		if (now) {
			if (adj < 0) {
				SERIALSTR("<sub:");
				serial_send_char(me->time.ht);
				serial_send_char(me->time.h1);
				serial_send_char(me->time.mt);
				serial_send_char(me->time.m1);
				SERIALSTR(":");
				serial_send_int(adj);
				SERIALSTR(">");

				/* Retard the clock so it runs slower. */
				BSP_sub_8th_second();
			} else {
				SERIALSTR("<add:");
				serial_send_char(me->time.ht);
				serial_send_char(me->time.h1);
				serial_send_char(me->time.mt);
				serial_send_char(me->time.m1);
				SERIALSTR(":");
				serial_send_int(adj);
				SERIALSTR(">");

				/* Advance the clock so it runs faster. */
				BSP_add_8th_second();
			}
		}
	}
}



static QState counting(struct RTC *me)
{
	enum time_wrap w;

	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		post((QActive*)(&ui), TIME_SIGNAL, (QParam)(&(me->time)));
		QActive_armX((QActive*)me, 0, 1);
		return Q_HANDLED();
	case Q_TIMEOUT_SIG:
		w = tick_time(&(me->time));
		QActive_post(&(ui.super), TIME_SIGNAL, (QParam)(&(me->time)));
		QActive_armX((QActive*)me, 0, 1);
		if (w) {
			SERIALSTR("\r\nTIME=");
			serial_send_char(me->time.ht);
			serial_send_char(me->time.h1);
			serial_send_char(me->time.mt);
			serial_send_char(me->time.m1);
			SERIALSTR("\r\n");
			do_adjustment(me);
		}
		return Q_HANDLED();
	}
	return Q_SUPER(&QHsm_top);
}
