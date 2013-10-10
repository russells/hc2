#ifndef bsp_h_INCLUDED
#define bsp_h_INCLUDED

#include "qpn_port.h"

// TODO: calculate ticks per second correctly.
#define BSP_TICKS_PER_SECOND 1

void BSP_init(void);
void BSP_led_on(void);
void BSP_led_off(void);


void BSP_enable_morse_line(void);
void BSP_morse_signal(uint8_t onoff);
void BSP_stop_everything(void);
void BSP_do_reset(void);

void BSP_start_temperature_reading(void);

#ifdef __MSP430__
#define BSP_delay_ms(ms) __delay_cycles(1000L*ms)
#endif


#endif

