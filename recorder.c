#include "recorder.h"
#include "time.h"
#include "ui.h"
#include "rtc.h"
#include "bsp.h"
#include "bsp-temperature-scale.h"
#include "serial.h"
#include "truefalse.h"

Q_DEFINE_THIS_MODULE("rec");


struct Recorder recorder;

static QState initial(struct Recorder *me);
static QState top(struct Recorder *me);
static QState waiting(struct Recorder *me);
static QState recTemperature(struct Recorder *me);
static QState recGetTemperature(struct Recorder *me);
static int16_t get_calibrated_ti(int16_t ti, int16_t cal);
static void save_max_and_min(struct Recorder *me, int16_t ti);
static void save_extreme(struct TiTime *today, struct TiTime *yesterday,
			 const struct Time *currenttime,
			 const struct Time *changeovertime,
			 int16_t ti,
			 uint8_t (*compare)(int16_t now, int16_t ext));
static uint8_t compare_max(int16_t now, int16_t max);
static uint8_t compare_min(int16_t now, int16_t min);


static const struct Time time_9am = { 2014, 3, 7, '0', '9', '0', '0', 0 };
static const struct Time time_3pm = { 2014, 3, 7, '1', '5', '0', '0', 0 };


void recorder_ctor(void)
{
	QActive_ctor(&recorder.super, (QStateHandler)(&initial));
	recorder.ti = INVALIDTI;
	recorder.max_today.ti = INVALIDTI;
	recorder.max_yesterday.ti = INVALIDTI;
	recorder.min_today.ti = INVALIDTI;
	recorder.min_yesterday.ti = INVALIDTI;
	recorder.calibration = BSP_get_calibration();
}


static QState initial(struct Recorder *me)
{
	return Q_TRAN(&waiting);
}


static QState top(struct Recorder *me)
{
	return Q_SUPER(&QHsm_top);
}


static QState waiting(struct Recorder *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		SERIALSTR("<W>");
		BSP_fast_timer_1(FALSE);
		QActive_arm(&me->super, 1);
		return Q_HANDLED();
	case Q_TIMEOUT_SIG:
		return Q_TRAN(recTemperature);
	}
	return Q_SUPER(top);
}


// TODO move all the temperature code from ui.c into here.


static QState recTemperature(struct Recorder *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		SERIALSTR("<R>");
		BSP_fast_timer_1(TRUE);
		// Start reading the temperature.
		BSP_start_temperature_reading();
		// If we time out, go back to just waiting.
		QActive_armX(&me->super, 1, 1);
		return Q_HANDLED();
	case Q_TIMEOUT1_SIG:
		return Q_TRAN(recGetTemperature);
	}
	return Q_SUPER(top);
}


static QState recGetTemperature(struct Recorder *me)
{
	int16_t ti;

	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		SERIALSTR("<G>");
		BSP_get_temperature();
		QActive_armX((QActive*)me, 1, 2);
		return Q_HANDLED();
	case TEMPERATURE_SIGNAL:
		ti = (int16_t) Q_PAR(me);
		me->ti = get_calibrated_ti(ti, me->calibration);
		save_max_and_min(me, me->ti);
		SERIALSTR("(TI:");
		serial_send_int(me->max_today.ti);
		SERIALSTR(":");
		serial_send_int(me->min_today.ti);
		SERIALSTR(":");
		serial_send_int(me->max_yesterday.ti);
		SERIALSTR(":");
		serial_send_int(me->min_yesterday.ti);
		SERIALSTR(")");
		post(&ui.super, CURRENT_TEMPERATURE_SIGNAL, (QParam) me->ti);
		return Q_TRAN(waiting);
	case Q_TIMEOUT1_SIG:
		return Q_TRAN(waiting);
	}
	return Q_SUPER(recTemperature);
}


static void save_max_and_min(struct Recorder *me, int16_t ti)
{
	save_extreme(&me->min_today, &me->min_yesterday,
		     gettimep(), &time_9am, ti, compare_min);

	save_extreme(&me->max_today, &me->max_yesterday,
		     gettimep(), &time_3pm, ti, compare_max);
}


/**
 * Save an extreme value.
 *
 * @param today the time and current extreme value for today
 *
 * @param yesterday the time and current extreme value for yesterday
 *
 * @param currenttime now
 *
 * @param changeovertime the time of day that is used to change between
 * recording days.  At that time, the current day's value becomes yesterday,
 * and we start recording for a new day.
 *
 * @param ti the current temperature value (right now).  This gets compared
 * with today's extreme value to see if it is the new extreme value.
 *
 * @param compare a function used to see if the current value (ti) is the new
 * extreme.  Takes two parameters, the current value and the current extreme
 * value, and returns true if the current value is the new extreme.
 */
static void save_extreme(struct TiTime *today, struct TiTime *yesterday,
			 const struct Time *currenttime,
			 const struct Time *changeovertime,
			 int16_t ti,
			 uint8_t (*compare)(int16_t now, int16_t ext))
{
	/* First decide if we've just changed over to the next recording
	   period. compare_times() ignores the date parts. */
	if ( ! compare_times(currenttime, changeovertime) ) {
		*yesterday = *today;
		today->ti = INVALIDTI;
	}
	if ( (INVALIDTI == today->ti) || (*compare)(ti, today->ti) ) {
		today->ti = ti;
	}
}


static uint8_t compare_max(int16_t now, int16_t max)
{
	return now > max;
}


static uint8_t compare_min(int16_t now, int16_t min)
{
	return now < min;
}


static int16_t get_calibrated_ti(int16_t ti, int16_t cal)
{
	int16_t adjti;

	SERIALSTR("adj:");
	serial_send_int(ti);
	SERIALSTR(":");
	/* If the temperature value is outside the BSP range, leave it
	   there. */
	if ((LOWTI == ti) || (HIGHTI == ti)) {
		adjti = ti;
	} else {
		adjti = ti + cal;
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


void set_calibration(int16_t cal)
{
	recorder.calibration = cal;
	BSP_save_calibration(cal);
}
