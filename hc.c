
#include "qpn_port.h"
#include "hc.h"
#include "buttons.h"
#include "bsp.h"
#include "lcd.h"
#include "rtc.h"
#include "ui.h"
#include "serial.h"


Q_DEFINE_THIS_FILE;


static QEvent rtcQueue[4];
/** The size of buttonsQueue[] is embarrassing, and reveals a flaw in the
    current code.  It will go away completely in a later iteration. */
static QEvent buttonsQueue[18];
static QEvent uiQueue[8];

QActiveCB const Q_ROM Q_ROM_VAR QF_active[] = {
	{ (QActive*)0           , (QEvent*)0   , 0                   },
	{ (QActive *) (&rtc)    , rtcQueue     , Q_DIM(rtcQueue)     },
	{ (QActive *) (&buttons), buttonsQueue , Q_DIM(buttonsQueue) },
	{ (QActive *) (&ui)     , uiQueue      , Q_DIM(uiQueue)      },
};
Q_ASSERT_COMPILE( QF_MAX_ACTIVE == Q_DIM(QF_active) - 1 );


int main(void)
{
 startmain:
	BSP_init();
	serial_init();
	SERIALSTR("\r\n\r\n*** Hots and Colds ***\r\n");
	lcd_init();
	ui_ctor();
	buttons_ctor();
	rtc_ctor();
	SERIALSTR("Let's go...\r\n");
	QF_run();
	goto startmain;

	return 0;
}
