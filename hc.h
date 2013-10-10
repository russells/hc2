#ifndef hc_h_INCLUDED
#define hc_h_INCLUDED

#include "qpn_port.h"

struct Hc {
	QActive super;
	//uint8_t lcdchar;
	uint8_t temperature;
	uint8_t hot;
};


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
