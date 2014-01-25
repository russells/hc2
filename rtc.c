#include "rtc.h"
#include "ui.h"
#include "lcd.h"

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


void rtc_ctor(void)
{
	QActive_ctor((QActive*)(&rtc), (QStateHandler)(&initial));
	rtc.time.year = 2014;
	rtc.time.month = 3;
	rtc.time.day = 7;
	/* TODO make this default to the time of the ceremony. */
	rtc.time.ht = '2';
	rtc.time.h1 = '2';
	rtc.time.mt = '0';
	rtc.time.m1 = '0';
	rtc.time.seconds = 0;
}


static QState initial(struct RTC *me)
{
	return Q_TRAN(counting);
}


static QState counting(struct RTC *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		post((QActive*)(&ui), TIME_SIGNAL, (QParam)(&(me->time)));
		QActive_armX((QActive*)me, 0, 1);
		return Q_HANDLED();
	case Q_TIMEOUT_SIG:
		tick_time(&(me->time));
		QActive_armX((QActive*)me, 0, 1);
		return Q_HANDLED();
	}
	return Q_SUPER(&QHsm_top);
}
