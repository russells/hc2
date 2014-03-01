#ifndef ui_h_INCLUDED
#define ui_h_INCLUDED

#include "qpn_port.h"
#include "hc.h"
#include "time.h"

struct UI {
	QActive super;
	/** Temperature as returned by the BSP.  It is scaled by a factor of
	    two, so we can show temperature to half a degree. */
	int16_t ti;
	int16_t cal;
	int16_t adjustment;	/* Time adjustment, seconds/day. */

	uint8_t timeoutcounter;

	uint8_t scrollindex;
	const char *scrollstring;
	char scrolltext[8];

	/**
	 * The time that the user is setting.  This gets saved back to the RTC
	 * depending on what the user selects at the end of the process.
	 */
	struct Time settime;

	/**
	 * Set to indicate the part of the time that the user is setting.
	 */
	char settime_YmdHM;

	/**
	 * Used to implement a simple informal linear state machine when we are
	 * displaying the maximum or minimum temperatures.
	 */
	uint8_t showmaxmincounter;

	/**
	 * Count the number of times we try to get the temperature reading.
	 */
	uint8_t temperatureWaits;
};


extern struct UI ui;

void ui_ctor(void);

#endif
