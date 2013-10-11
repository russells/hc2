
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
static QState hcPause(struct Hc *me);
static QState hcTemperature(struct Hc *me);
static QState hcGetTemperature(struct Hc *me);

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
}


static QState hcInitial(struct Hc *me)
{
	return Q_TRAN(hcTemperature);
}


static QState hcTop(struct Hc *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		lcd_clear();
		break;
	case WATCHDOG_SIGNAL:
		// TODO: watchdog reset
		return Q_HANDLED();
	case TEMPERATURE_SIGNAL:
		SERIALSTR("hcTop: TS\r\n");
		return Q_HANDLED();
	}
	return Q_SUPER(QHsm_top);
}


static QState hcPause(struct Hc *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		SERIALSTR("hcPause\r\n");
		BSP_slow_timer();
		QActive_arm((QActive*)me, 1);
		return Q_HANDLED();
	case Q_TIMEOUT_SIG:
		return Q_TRAN(hcTemperature);
	}
	return Q_SUPER(hcTop);
}


static QState hcTemperature(struct Hc *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		SERIALSTR("hcTemp\r\n");
		BSP_fast_timer();
		// Start reading the temperature.
		BSP_start_temperature_reading();
		// If we time out, go back to just waiting.
		QActive_arm((QActive*)me, 1);
		return Q_HANDLED();
	case Q_TIMEOUT_SIG:
		return Q_TRAN(hcGetTemperature);
	}
	return Q_SUPER(hcTop);
}


static QState hcGetTemperature(struct Hc *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		SERIALSTR("hcGet\r\n");
		BSP_get_temperature();
		QActive_arm((QActive*)me, 2);
		return Q_HANDLED();
	case TEMPERATURE_SIGNAL:
		show_temperature(me, (int8_t) Q_PAR(me));
		return Q_TRAN(hcPause);
	case Q_TIMEOUT_SIG:
		return Q_TRAN(hcPause);
	}
	return Q_SUPER(hcTemperature);
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
