#ifndef rtc_h_INCLUDED
#define rtc_h_INCLUDED

#include "qpn_port.h"
#include "hc.h"

struct RTC {
	QActive super;
	char ht;		/** Hour tens */
	char h1;		/** Hour units */
	char mt;		/** Minute tens */
	char m1;		/** Minute units */
	uint8_t seconds;	/** Count of seconds within a minute */
};


extern struct RTC rtc;

void rtc_ctor(void);

#endif
