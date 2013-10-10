
#include "qpn_port.h"
#include <inttypes.h>
#include <string.h>
#include "hc.h"
#include "bsp.h"
#include "lcd.h"
#include "serial.h"


Q_DEFINE_THIS_FILE;


struct Hc hc;

static void hc_ctor(void);
static QState hcInitial(struct Hc *me);
static QState hcTop(struct Hc *me);
static QState lcdSequence(struct Hc *me);
static QState lcdAllOn(struct Hc *me);

static void show_temperature(struct Hc *me, int8_t ti);


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
	serial_init();
	SERIALSTR("Hots'n'Colds\r\n");
	lcd_init();
	hc_ctor();
	SERIALSTR("Let's go...\r\n");
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
	case TEMPERATURE_SIGNAL:
		//Q_ASSERT(0);
		break;
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
		serial_send(display);
		SERIALSTR("\r\n");
		lcd_showstring(display, 0);
		me->temperature ++;
		QActive_arm((QActive*)me, BSP_TICKS_PER_SECOND);
		BSP_start_temperature_reading();
		return Q_HANDLED();
	case TEMPERATURE_SIGNAL:
		show_temperature(me, (int8_t) Q_PAR(me));
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
		SERIALSTR("all on\r\n");
		QActive_arm((QActive*)me, BSP_TICKS_PER_SECOND);
		return Q_HANDLED();
	case Q_TIMEOUT_SIG:
		return Q_TRAN(lcdSequence);
	}
	return Q_SUPER(hcTop);
}


const char tstrings[141][8] = {
	"30COLDS", "29COLDS", "28COLDS", "27COLDS", "26COLDS",
	"25COLDS", "24COLDS", "23COLDS", "22COLDS", "21COLDS",
	"20COLDS", "19COLDS", "18COLDS", "17COLDS", "16COLDS",
	"15COLDS", "14COLDS", "13COLDS", "12COLDS", "11COLDS",
	"10COLDS", "9 COLDS", "8 COLDS", "7 COLDS", "6 COLDS",
	"5 COLDS", "4 COLDS", "3 COLDS", "2 COLDS", "1 COLD ",
	" NORMAL",
	"1  HOT ", "2  HOTS", "3  HOTS", "4  HOTS", "5  HOTS",
	"6  HOTS", "7  HOTS", "8  HOTS", "9  HOTS", "10 HOTS",
	"11 HOTS", "12 HOTS", "13 HOTS", "14 HOTS", "15 HOTS",
	"16 HOTS", "17 HOTS", "18 HOTS", "19 HOTS", "20 HOTS",
	"21 HOTS", "22 HOTS", "23 HOTS", "24 HOTS", "25 HOTS",
	"26 HOTS", "27 HOTS", "28 HOTS", "29 HOTS", "30 HOTS",
	"31 HOTS", "32 HOTS", "33 HOTS", "34 HOTS", "35 HOTS",
	"36 HOTS", "37 HOTS", "38 HOTS", "39 HOTS", "40 HOTS",
};


static void show_temperature(struct Hc *me, int8_t ti)
{
	ti += 20;
	Q_ASSERT( ti >= 0 );
	Q_ASSERT( ti <= 70 );
	ti /= 2;
	lcd_showstring(tstrings[ti], 0);
}
