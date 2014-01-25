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
	uint8_t seconds;	/** Count of seconds within a minute */
};


/** Increments the whole time, starting from seconds and cascading through
    minutes, hours, days, months, and years. */
void tick_time(struct Time *time);

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

#endif
