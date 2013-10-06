
#include "qpn_port.h"
#include <inttypes.h>
#include <string.h>
#include "hc.h"
#include "bsp.h"
#include "lcd.h"


Q_DEFINE_THIS_FILE;


struct Hc hc;

static void hc_ctor(void);
static QState hcInitial(struct Hc *me);
static QState hcTop(struct Hc *me);
static QState lcdSequence(struct Hc *me);
static QState lcdAllOn(struct Hc *me);


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
	lcd_init();
	hc_ctor();
	QF_run();
	goto startmain;

	return 0;
}


static void hc_ctor(void)
{
	QActive_ctor((QActive*)(&hc), (QStateHandler)(&hcInitial));
	hc.hot = 1;
	hc.temperature = 0;
}


static QState hcInitial(struct Hc *me)
{
	return Q_TRAN(lcdAllOn);
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


static QState lcdSequence(struct Hc *me)
{
	static const char hots [] = "   HOTS";
	static const char colds[] = "  COLDS";
	char display[8];

	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		lcd_clear();
		if (me->temperature > 9) {
			me->temperature = 0;
			me->hot = ! me->hot;
		}
		if (me->hot) {
			strncpy(display, hots, 8);
		} else {
			strncpy(display, colds, 8);
		}
		display[7] = '\0';
		display[0] = me->temperature + '0';
		if (1 == me->temperature) {
			display[6] = ' ';
		}
		lcd_showstring(display, 0);
		me->temperature ++;
		QActive_arm((QActive*)me, BSP_TICKS_PER_SECOND);
		return Q_HANDLED();
	case Q_TIMEOUT_SIG:
		return Q_TRAN(lcdAllOn);
	}
	return Q_SUPER(hcTop);
}


static QState lcdAllOn(struct Hc *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		//lcd_show("On");
		lcd_show("\xff");
		//BSP_led_on();
		QActive_arm((QActive*)me, BSP_TICKS_PER_SECOND);
		return Q_HANDLED();
	case Q_TIMEOUT_SIG:
		return Q_TRAN(lcdSequence);
	}
	return Q_SUPER(hcTop);
}
