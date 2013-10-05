
#include "qpn_port.h"
#include <inttypes.h>
#include "hc.h"
#include "bsp.h"


Q_DEFINE_THIS_FILE;


struct Hc hc;

static void hc_ctor(void);
static QState hcInitial(struct Hc *me);
static QState hcTop(struct Hc *me);
static QState hcLedOff(struct Hc *me);
static QState hcLedOn(struct Hc *me);



static QEvent hcQueue[4];

QActiveCB const Q_ROM Q_ROM_VAR QF_active[] = {
	{ (QActive*)0           , (QEvent*)0   , 0                   },
	{ (QActive *) (&hc)     , hcQueue      , Q_DIM(hcQueue)      },
};
Q_ASSERT_COMPILE( QF_MAX_ACTIVE == Q_DIM(QF_active) - 1 );


int main(void)
{
 startmain:
	BSP_init();
	hc_ctor();
	QF_run();
	goto startmain;

	return 0;
}


static void hc_ctor(void)
{
	QActive_ctor((QActive*)(&hc), (QStateHandler)(&hcInitial));
}


static QState hcInitial(struct Hc *me)
{
	return Q_TRAN(hcLedOff);
}


static QState hcTop(struct Hc *me)
{
	switch (Q_SIG(me)) {
	case WATCHDOG_SIGNAL:
		// TODO: watchdog reset
		return Q_HANDLED();
	}
	return Q_SUPER(QHsm_top);
}


static QState hcLedOff(struct Hc *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		//BSP_led_off();
		QActive_arm((QActive*)me, BSP_TICKS_PER_SECOND);
		return Q_HANDLED();
	case Q_TIMEOUT_SIG:
		return Q_TRAN(hcLedOn);
	}
	return Q_SUPER(hcTop);
}


static QState hcLedOn(struct Hc *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		//BSP_led_on();
		QActive_arm((QActive*)me, BSP_TICKS_PER_SECOND);
		return Q_HANDLED();
	case Q_TIMEOUT_SIG:
		return Q_TRAN(hcLedOff);
	}
	return Q_SUPER(hcTop);
}
