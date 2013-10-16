#ifndef hc_h_INCLUDED
#define hc_h_INCLUDED

#include "qpn_port.h"
#include <limits.h>

struct Hc {
	QActive super;
	/** Temperature as returned by the BSP.  It is scaled by a factor of
	    two, so we can show temperature to half a degree. */
	int16_t ti;
	int16_t calibration;
};


/** The minimum value for calibration. */
#define MIN_CAL -9
/** The maximum value for calibration. */
#define MAX_CAL 9


#define INVALIDTI INT_MIN


extern struct Hc hc;


enum HcSignals {
	WATCHDOG_SIGNAL = Q_USER_SIG,
	/**
	 * The argument to TEMPERATURE_SIGNAL (Q_SIG(me)), is the encoded
	 * temperature.  The coding is in a signed byte (int8_t), with a
	 * resolution of two LSBs per degree C.  So 0==0C, 1==0.5C, 2==1C,
	 * 4==2C, and -1==-0.5C, -2==-1C.  40==20C=="NORMAL".
	 */
	TEMPERATURE_SIGNAL,
	MAX_PUB_SIG,
	MAX_SIG,
};

#endif
