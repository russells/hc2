#ifndef hc_h_INCLUDED
#define hc_h_INCLUDED

#include "qpn_port.h"
#include <limits.h>


/** The minimum value for calibration. */
#define MIN_CAL -9

/** The maximum value for calibration. */
#define MAX_CAL 9

/** A value that specifies an invalid temperature. */
#define INVALIDTI INT_MIN


enum HcSignals {
	WATCHDOG_SIGNAL = Q_USER_SIG,
	/**
	 * The argument to TEMPERATURE_SIGNAL (Q_SIG(me)), is the encoded
	 * temperature.  The coding is in a signed byte (int8_t), with a
	 * resolution of two LSBs per degree C.  So 0==0C, 1==0.5C, 2==1C,
	 * 4==2C, and -1==-0.5C, -2==-1C.  40==20C=="NORMAL".
	 */
	TEMPERATURE_SIGNAL,

	/**
	 * Sent to the UI when the time changes.  Q_PAR() is a pointer to four
	 * characters ('0' to '9') to display on the time section.
	 */
	TIME_SIGNAL,

	/**
	 * Send from Buttons to its constituent Button HSMs to tell them that
	 * their button is pressed.
	 */
	BUTTON_PRESSED_SIGNAL,

	/**
	 * Sent from Buttons to its constituent Button HSMs to tell them that
	 * their button is not pressed.
	 */
	BUTTON_RELEASED_SIGNAL,

	/** Button 1 is held down. */
	B_1_DOWN_SIGNAL,
	/** Button 2 is held down. */
	B_2_DOWN_SIGNAL,
	/** Button 3 is held down. */
	B_3_DOWN_SIGNAL,

	/** Button 1 is not held down. */
	B_1_UP_SIGNAL,
	/** Button 2 is not held down. */
	B_2_UP_SIGNAL,
	/** Button 3 is not held down. */
	B_3_UP_SIGNAL,

	BUTTON_1_PRESS_SIGNAL,
	BUTTON_1_LONG_PRESS_SIGNAL,
	BUTTON_1_REPEAT_SIGNAL,
	BUTTON_1_RELEASE_SIGNAL,

	BUTTON_2_PRESS_SIGNAL,
	BUTTON_2_LONG_PRESS_SIGNAL,
	BUTTON_2_REPEAT_SIGNAL,
	BUTTON_2_RELEASE_SIGNAL,

	BUTTON_3_PRESS_SIGNAL,
	BUTTON_3_LONG_PRESS_SIGNAL,
	BUTTON_3_REPEAT_SIGNAL,
	BUTTON_3_RELEASE_SIGNAL,

	MAX_PUB_SIG,
	MAX_SIG,
};


#define post(me_,sig_,par_)                                            \
	do {							       \
		const QActive *ao = (QActive*)me_;		       \
		QActiveCB const Q_ROM *acb = &QF_active[ao->prio];     \
		uint8_t end = Q_ROM_BYTE(acb->end);		       \
		QF_INT_DISABLE();				       \
		Q_ASSERT( (end - ao->nUsed) > 0);		       \
		QF_INT_ENABLE();				       \
		QActive_post((QActive*)(me_), sig_, par_);	       \
	} while (0);


#define postISR(me_,sig_,par_)                                         \
	do {							       \
		const QActive *ao = (QActive*)me_;		       \
		QActiveCB const Q_ROM *acb = &QF_active[ao->prio];     \
		uint8_t end = Q_ROM_BYTE(acb->end);		       \
		Q_ASSERT( (end - ao->nUsed) > 0 );		       \
		QActive_postISR((QActive*)(me_), sig_, par_);	       \
	} while (0);



#endif
