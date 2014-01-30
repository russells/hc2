#ifndef time_h_INCLUDED
#define time_h_INCLUDED

#include <stdint.h>

struct Time {
	uint16_t year;		/** Year */
	uint8_t month;		/** Month */
	uint8_t day;		/** Day */
	char ht;		/** Hour tens */
	char h1;		/** Hour units */
	char mt;		/** Minute tens */
	char m1;		/** Minute units */
	int8_t seconds;		/** Count of seconds within a minute.  This is
				    an int8_t rather than uint8_t so we can
				    subtract from it at the end of each day for
				    adjustment. */
};


enum time_wrap {
	wrap_none = 0,
	wrap_minute,
	wrap_hour,
	wrap_day,
	wrap_month,
	wrap_year,
};


/** Increments the whole time, starting from seconds and cascading through
    minutes, hours, days, months, and years. */
enum time_wrap tick_time(struct Time *time);

void inc_year(struct Time *time);
uint8_t inc_month(struct Time *time);
uint8_t inc_day(struct Time *time);
uint8_t inc_hour(struct Time *time);
uint8_t inc_minute(struct Time *time);

void dec_year(struct Time *time);
void dec_month(struct Time *time);
void dec_day(struct Time *time);
void dec_hour(struct Time *time);
void dec_minute(struct Time *time);

uint8_t max_day_for_month(struct Time *time);

int8_t compare_times(const struct Time *t1, const struct Time *t2);
int8_t compare_dates(const struct Time *t1, const struct Time *t2);
int8_t compare_time(struct Time *t1, struct Time *t2);

#endif
