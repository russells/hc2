#ifndef buttons_h_INCLUDED
#define buttons_h_INCLUDED

#include "qpn_port.h"
#include "hc.h"


struct Button {
	QHsm super;
	int press_signal;
	int long_press_signal;
	int repeat_signal;
	int release_signal;
	uint8_t counter;
};


struct Buttons {
	QActive super;
	struct Button button1;
	struct Button button2;
	struct Button button3;
};


extern struct Buttons buttons;

void buttons_ctor(void);

#endif
