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
static void inc_time(struct RTC *me);


struct RTC rtc;


void rtc_ctor(void)
{
	QActive_ctor((QActive*)(&rtc), (QStateHandler)(&initial));
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
		QActive_post((QActive*)(&ui), TIME_SIGNAL, (QParam)(&(me->time)));
		QActive_armX((QActive*)me, 0, 1);
		return Q_HANDLED();
	case Q_TIMEOUT_SIG:
		inc_time(me);
		QActive_armX((QActive*)me, 0, 1);
		return Q_HANDLED();
	}
	return Q_SUPER(&QHsm_top);
}


static void inc_time(struct RTC *me)
{
	me->time.seconds += 2;

	if (me->time.seconds >= 60U) {
		me->time.seconds = 0;
		me->time.m1 ++;
		if (me->time.m1 > '9') {
			me->time.m1 = '0';
			me->time.mt ++;
			if (me->time.mt > '5') {
				me->time.mt = '0';
				if (me->time.h1 == '3' && me->time.ht == '2') {
					me->time.h1 = '0';
					me->time.ht = '0';
				} else {
					me->time.h1 ++;
					if (me->time.h1 > '9') {
						me->time.h1 = '0';
						me->time.ht ++;
					}
				}
			}
		}
	}
	QActive_post((QActive*)(&ui), TIME_SIGNAL, (QParam)(&(me->time)));
}
