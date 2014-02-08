#include "buttons.h"
#include "hc.h"
#include "ui.h"
#include "serial.h"


Q_DEFINE_THIS_MODULE("btn");


static QState buttons_initial(struct Buttons *me);
static QState buttons_state(struct Buttons *me);
static QState buttons_waiting(struct Buttons *me);


static QState button_initial(struct Button *me);
static QState button_up(struct Button *me);
static QState button_maybe_down(struct Button *me);
static QState button_down(struct Button *me);
static QState button_long(struct Button *me);


struct Buttons buttons;


void buttons_ctor(void)
{
	QActive_ctor((QActive*)(&buttons), (QStateHandler)buttons_initial);

	SERIALSTR("(b1)");

	QHsm_ctor((QHsm*)(&(buttons.button1)), (QStateHandler)button_initial);
	buttons.button1.press_signal = BUTTON_1_PRESS_SIGNAL;
	buttons.button1.long_press_signal = BUTTON_1_LONG_PRESS_SIGNAL;
	buttons.button1.repeat_signal = BUTTON_1_REPEAT_SIGNAL;
	buttons.button1.release_signal = BUTTON_1_RELEASE_SIGNAL;
	QHsm_init((QHsm*)(&(buttons.button1)));

	QHsm_ctor((QHsm*)(&(buttons.button2)), (QStateHandler)button_initial);
	buttons.button2.press_signal = BUTTON_2_PRESS_SIGNAL;
	buttons.button2.long_press_signal = BUTTON_2_LONG_PRESS_SIGNAL;
	buttons.button2.repeat_signal = BUTTON_2_REPEAT_SIGNAL;
	buttons.button2.release_signal = BUTTON_2_RELEASE_SIGNAL;
	QHsm_init((QHsm*)(&(buttons.button2)));

	QHsm_ctor((QHsm*)(&(buttons.button3)), (QStateHandler)button_initial);
	buttons.button3.press_signal = BUTTON_3_PRESS_SIGNAL;
	buttons.button3.long_press_signal = BUTTON_3_LONG_PRESS_SIGNAL;
	buttons.button3.repeat_signal = BUTTON_3_REPEAT_SIGNAL;
	buttons.button3.release_signal = BUTTON_3_RELEASE_SIGNAL;
	QHsm_init((QHsm*)(&(buttons.button3)));

}



static QState buttons_initial(struct Buttons *me)
{
	return Q_TRAN(buttons_state);
}


#define DS(button_, sig_)				\
	do {						\
		Q_SIG(&(me->button_)) = (sig_);		\
		Q_PAR(&(me->button_)) = (0);		\
		QHsm_dispatch((QHsm*)(&(me->button_)));	\
	} while (0)

static QState buttons_state(struct Buttons *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		SERIALSTR("(b2)");
		return Q_HANDLED();
	case BUTTONS_WAIT_SIGNAL:
		return Q_TRAN(buttons_waiting);
	case BUTTONS_SIGNAL:
		if ((uint16_t)(Q_PAR(me)) & 0x1) {
			DS(button1, BUTTON_PRESSED_SIGNAL);
		} else {
			DS(button1, BUTTON_RELEASED_SIGNAL);
		}
		if ((uint16_t)(Q_PAR(me)) & 0x2) {
			DS(button2, BUTTON_PRESSED_SIGNAL);
		} else {
			DS(button2, BUTTON_RELEASED_SIGNAL);
		}
		if ((uint16_t)(Q_PAR(me)) & 0x4) {
			DS(button3, BUTTON_PRESSED_SIGNAL);
		} else {
			DS(button3, BUTTON_RELEASED_SIGNAL);
		}
		return Q_HANDLED();
	}
	return Q_SUPER(QHsm_top);
}


static QState buttons_waiting(struct Buttons *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		SERIALSTR("<+bw>");
		DS(button1, BUTTON_RELEASED_SIGNAL);
		DS(button2, BUTTON_RELEASED_SIGNAL);
		DS(button3, BUTTON_RELEASED_SIGNAL);
		return Q_HANDLED();
	case BUTTONS_SIGNAL:
		if ((uint16_t)(Q_PAR(me)) == 0) {
			SERIALSTR("@");
			return Q_TRAN(buttons_state);
		} else {
			return Q_HANDLED();
		}
	case Q_EXIT_SIG:
		SERIALSTR("<-bw>");
		return Q_HANDLED();
	}
	return Q_SUPER(buttons_state);
}


static QState button_initial(struct Button *me)
{
	return Q_TRAN(button_up);
}


static QState button_up(struct Button *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		me->counter = 0;
		return Q_HANDLED();
	case BUTTON_PRESSED_SIGNAL:
		me->counter = 1;
		return Q_TRAN(button_maybe_down);
	}
	return Q_SUPER(QHsm_top);
}


static QState button_maybe_down(struct Button *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		return Q_HANDLED();
	case BUTTON_PRESSED_SIGNAL:
		me->counter ++;
		if (me->counter == 3) {
			return Q_TRAN(button_down);
		} else {
			return Q_HANDLED();
		}
	case BUTTON_RELEASED_SIGNAL:
		return Q_TRAN(button_up);
	}
	return Q_SUPER(button_up);
}


static QState button_down(struct Button *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		post((QActive*)(&ui), me->press_signal, 0);
		return Q_HANDLED();
	case BUTTON_PRESSED_SIGNAL:
		me->counter ++;
		if (me->counter == 32) {
			return Q_TRAN(button_long);
		} else {
			return Q_HANDLED();
		}
	case Q_EXIT_SIG:
		post((QActive*)(&ui), me->release_signal, 0);
		return Q_HANDLED();
	}
	return Q_SUPER(button_maybe_down);
}


static QState button_long(struct Button *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		post((QActive*)(&ui), me->long_press_signal, 0);
		return Q_HANDLED();
	case BUTTON_PRESSED_SIGNAL:
		me->counter ++;
		if (me->counter == 47) {
			post((QActive*)(&ui), me->repeat_signal, 0);
			me->counter = 32;
		}
		return Q_HANDLED();
	}
	return Q_SUPER(button_down);
}
