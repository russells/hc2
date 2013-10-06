#ifndef hc_h_INCLUDED
#define hc_h_INCLUDED

#include "qpn_port.h"

struct Hc {
	QActive super;
	//uint8_t lcdchar;
	uint8_t chindex;
	uint8_t digit;
};


extern struct Hc hc;


enum HcSignals {
	WATCHDOG_SIGNAL = Q_USER_SIG,
	MAX_PUB_SIG,
	MAX_SIG,
};

#endif
