#ifndef ui_h_INCLUDED
#define ui_h_INCLUDED

#include "qpn_port.h"
#include "hc.h"

struct UI {
	QActive super;
	/** Temperature as returned by the BSP.  It is scaled by a factor of
	    two, so we can show temperature to half a degree. */
	int16_t ti;
	int16_t calibration;

	uint8_t scrollindex;
	const char *scrollstring;
	char scrolltext[8];
};


extern struct UI ui;

void ui_ctor(void);

#endif
