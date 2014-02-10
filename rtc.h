#ifndef rtc_h_INCLUDED
#define rtc_h_INCLUDED

#include "qpn_port.h"
#include "hc.h"
#include "time.h"

struct RTC {
	QActive super;
	struct Time time;
};


extern struct RTC rtc;

void rtc_ctor(void);

inline struct Time *gettimep(void) {
	return &(rtc.time);
}

void set_rtc_time(const struct Time *time);

#endif
