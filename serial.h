#ifndef serial_h_INCLUDED
#define serial_h_INCLUDED
#include "qpn_port.h"
#include <msp430.h>

#ifdef SERIAL

void serial_init(void);
int  serial_send(const char *s);
int  serial_send_int(unsigned int n);
int  serial_send_hex_int(unsigned int x);
int  serial_send_char(char c);
//void serial_assert(char const Q_ROM * const Q_ROM_VAR file, int line);
//void serial_assert_nostop(char const Q_ROM * const Q_ROM_VAR file, int line);
void serial_drain(void);

/**
 * Send a constant string (stored in ROM).
 */
#define SERIALSTR(s)				\
	do {					\
		static const char ss[] = s;	\
		serial_send(ss);		\
	} while (0)

#else

#define serial_init();
#define  serial_send(s);
#define  serial_send_int(n);
#define  serial_send_hex_int(x);
#define  serial_send_char(c);
//#define serial_assert(file, line)
//#define serial_assert_nostop(file, line)
#define serial_drain()
#define SERIALSTR(s)

#endif

#endif
