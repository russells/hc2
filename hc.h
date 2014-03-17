#ifndef hc_h_INCLUDED
#define hc_h_INCLUDED

#include "qpn_port.h"
#include <limits.h>


/** The minimum value for temperature calibration. */
#define MIN_CAL -9

/** The maximum value for temperature calibration. */
#define MAX_CAL 9

/** The maximum value for time adjustment.  The value of 80 allows for +/- 20
    seconds per day, at 0.25 second resolution. */
#define MAX_ADJ 80

/** The minimum value for time adjustment.  @see MAX_ADJ */
#define MIN_ADJ -80

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
	 * Sent from the UI to itself to tell that the user has done some
	 * action, and not to do the automatic timeout just yet.
	 */
	UI_ACTION_SIGNAL,

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

	/**
	 * Sent from the ISR to Buttons with a bitmask of buttons.
	 */
	BUTTONS_SIGNAL,

	/**
	 * Sent to Buttons to tell it that we have bypassed the normal button
	 * detection, and to wait for all buttons to be up before generating
	 * more button events.
	 */
	BUTTONS_WAIT_SIGNAL,

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

	BUTTON_4_PRESS_SIGNAL,
	BUTTON_4_LONG_PRESS_SIGNAL,
	BUTTON_4_REPEAT_SIGNAL,
	BUTTON_4_RELEASE_SIGNAL,

	/* The point of these is to map the real button names to the numbered
	   buttons.  The number of events for named buttons must exactly match
	   the number of events for numbered buttons. */

	BUTTON_ENTER_PRESS_SIGNAL       = BUTTON_1_PRESS_SIGNAL,
	BUTTON_ENTER_LONG_PRESS_SIGNAL  = BUTTON_1_LONG_PRESS_SIGNAL,
	BUTTON_ENTER_REPEAT_SIGNAL      = BUTTON_1_REPEAT_SIGNAL,
	BUTTON_ENTER_RELEASE_SIGNAL     = BUTTON_1_RELEASE_SIGNAL,

	BUTTON_DOWN_PRESS_SIGNAL        = BUTTON_2_PRESS_SIGNAL,
	BUTTON_DOWN_LONG_PRESS_SIGNAL   = BUTTON_2_LONG_PRESS_SIGNAL,
	BUTTON_DOWN_REPEAT_SIGNAL       = BUTTON_2_REPEAT_SIGNAL,
	BUTTON_DOWN_RELEASE_SIGNAL      = BUTTON_2_RELEASE_SIGNAL,

	BUTTON_UP_PRESS_SIGNAL          = BUTTON_3_PRESS_SIGNAL,
	BUTTON_UP_LONG_PRESS_SIGNAL     = BUTTON_3_LONG_PRESS_SIGNAL,
	BUTTON_UP_REPEAT_SIGNAL         = BUTTON_3_REPEAT_SIGNAL,
	BUTTON_UP_RELEASE_SIGNAL        = BUTTON_3_RELEASE_SIGNAL,

	BUTTON_CANCEL_PRESS_SIGNAL      = BUTTON_4_PRESS_SIGNAL,
	BUTTON_CANCEL_LONG_PRESS_SIGNAL = BUTTON_4_LONG_PRESS_SIGNAL,
	BUTTON_CANCEL_REPEAT_SIGNAL     = BUTTON_4_REPEAT_SIGNAL,
	BUTTON_CANCEL_RELEASE_SIGNAL    = BUTTON_4_RELEASE_SIGNAL,

	/**
	 * Sent to notify the UI of the current temperature.  Q_PAR() will be
	 * the temperature.
	 */
	CURRENT_TEMPERATURE_SIGNAL,

	MAX_PUB_SIG,
	MAX_SIG,
};


#define ASSERTION_BUFFER_MSG_SIZE 8 /* Sized for the LCD (7 chars + \0). */

struct AssertionBuffer {
	char valid[2];
	int line;
	char msg[ASSERTION_BUFFER_MSG_SIZE];
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
