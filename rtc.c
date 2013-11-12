#include "rtc.h"
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
	rtc.ht = '2';
	rtc.h1 = '2';
	rtc.mt = '0';
	rtc.m1 = '0';
	rtc.seconds = 0;
}


static QState initial(struct RTC *me)
{
	return Q_TRAN(counting);
}


static QState counting(struct RTC *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		lcd_showdigits(&(me->ht));
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
	me->seconds ++;
	if (me->seconds & 0x01) {
		lcd_colon(1);
	} else {
		lcd_colon(0);
	}

	if (me->seconds < 30U) {
		return;
	}

	me->seconds = 0;
	me->m1 ++;
	if (me->m1 > '9') {
		me->m1 = '0';
		me->mt ++;
		if (me->mt > '5') {
			me->mt = '0';
			if (me->h1 == '3' && me->ht == '2') {
				me->h1 = '0';
				me->ht = '0';
			} else {
				me->h1 ++;
				if (me->h1 > '9') {
					me->h1 = '0';
					me->ht ++;
				}
			}
		}
	}
	lcd_showdigits(&(me->ht));
}
