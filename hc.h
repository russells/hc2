#ifndef hc_h_INCLUDED
#define hc_h_INCLUDED

#include "qpn_port.h"

struct Hc {
	QActive super;
};


extern struct Hc hc;


enum HcSignals {
	WATCHDOG_SIGNAL = Q_USER_SIG,
	MAX_PUB_SIG,
	MAX_SIG,
};

#endif