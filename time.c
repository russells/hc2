#include "time.h"
#include "qpn_port.h"
#include "bsp.h"
#include "truefalse.h"

Q_DEFINE_THIS_MODULE("t");

/**
 *
 * The plan - have inc_second() etc up to inc_month().  They all return true if
 * there was a wrap around.  inc_year() does not have a return value.
 *
 * The normal tick code is in tick_time(), called once per second.
 * tick_second() calls inc_second().  If there was a wrap, call inc_minute().
 * If that wraps, call inc_hour(), etc.
 *
 *
 */


enum time_wrap tick_time(struct Time *time)
{
	enum time_wrap wrap = wrap_none;

	time->seconds += 2;
	if (time->seconds >= 60) {
		time->seconds = 0;
		wrap = wrap_second;
		if (inc_minute(time)) {
			wrap = wrap_minute;
			if (inc_hour(time)) {
				wrap = wrap_hour;
				if (inc_day(time)) {
					wrap = wrap_day;
					if (inc_month(time)) {
						wrap = wrap_month;
						inc_year(time);
					}
				}
			}
		}
	}
	return wrap;
}


uint8_t inc_minute(struct Time *time)
{
	uint8_t wrap = FALSE;

	time->m1 ++;
	if (time->m1 > '9') {
		time->m1 = '0';
		time->mt ++;
		if (time->mt > '5') {
			time->mt = '0';
			wrap = TRUE;
		}
	}
	return wrap;
}


uint8_t inc_hour(struct Time *time)
{
	uint8_t wrap = FALSE;

	if (time->h1 == '3' && time->ht == '2') {
		time->h1 = '0';
		time->ht = '0';
		wrap = TRUE;
	} else {
		time->h1 ++;
		if (time->h1 > '9') {
			time->h1 = '0';
			time->ht ++;
		}
	}
	return wrap;
}


uint8_t max_day_for_month(struct Time *time)
{
	uint8_t max_day;

	switch (time->month) {
	case 1:
	case 3:
	case 5:
	case 7:
	case 8:
	case 10:
	case 12:
		max_day = 31;
		break;
	case 2:
		/* We don't have to worry about unusual leap years (divisable
		   by 400) as we're already past 2000 and I'll be surprised if
		   this is working in the year 2400. Setting the date to before
		   2000-03-01 is user error, and the plan is to have the UI
		   prevent that anyway. */
		if (time->year % 4) {
			max_day = 28;
		} else {
			max_day = 29;
		}
		break;
	case 4:
	case 6:
	case 9:
	case 11:
		max_day = 30;
		break;
	default:
		max_day = 0;
		Q_ASSERT( 0 );
		break;
	}
	return max_day;
}


uint8_t inc_day(struct Time *time)
{
	uint8_t wrap = FALSE;
	uint8_t maxday = max_day_for_month(time);

	time->day ++;
	if (time->day > maxday) {
		time->day = 1;
		wrap = TRUE;
	}
	return wrap;
}


uint8_t inc_month(struct Time *time)
{
	uint8_t wrap = FALSE;

	time->month ++;
	if (time->month > 12) {
		time->month = 1;
		wrap = TRUE;
	}
	return wrap;
}


void inc_year(struct Time *time)
{
	if (time->year < MAX_YEAR)
		time->year ++;
}


void dec_year(struct Time *time)
{
	if (time->year > MIN_YEAR)
		time->year --;
}


void dec_month(struct Time *time)
{
	if (time->month == 1) {
		time->month = 12;
	} else {
		time->month --;
	}
}


void dec_day(struct Time *time)
{
	if (time->day == 1) {
		time->day = max_day_for_month(time);
	} else {
		time->day --;
	}
}


void dec_hour(struct Time *time)
{
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


void dec_minute(struct Time *time)
{
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


int8_t compare_times(const struct Time *t1, const struct Time *t2)
{
	if (t1->ht != t2->ht)
		return t1->ht - t2->ht;
	if (t1->h1 != t2->h1)
		return t1->h1 - t2->h1;
	if (t1->mt != t2->mt)
		return t1->mt - t2->mt;
	if (t1->m1 != t2->m1)
		return t1->m1 - t2->m1;
	return t1->seconds - t2->seconds;
}


int8_t compare_dates(const struct Time *t1, const struct Time *t2)
{
	if (t1->year != t2->year)
		return t1->year - t2->year;
	if (t1->month != t2->month)
		return t1->month - t2->month;
	return t1->day - t2->day;
}


int8_t compare_time(struct Time *t1, struct Time *t2)
{
	int8_t comp;
	comp = compare_dates(t1, t2);
	if (comp)
		return comp;
	else
		return compare_times(t1, t2);
}
