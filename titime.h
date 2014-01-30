#ifndef titime_h_INCLUDED
#define titime_h_INCLUDED

#include <stdint.h>
#include "time.h"

struct TiTime {
	int16_t ti;
	struct Time time;
};

#endif
