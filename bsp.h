#ifndef bsp_h_INCLUDED
#define bsp_h_INCLUDED

#include "qpn_port.h"

// TODO: calculate ticks per second correctly.
#define BSP_TICKS_PER_SECOND 1

void BSP_init(void);

void BSP_fast_timer_1(uint8_t onoff);
void BSP_fast_timer_2(uint8_t onoff);

#ifdef LED
void BSP_led_on(void);
void BSP_led_off(void);
#else
#define BSP_led_on()
#define BSP_led_off()
#endif


void BSP_enable_morse_line(void);
void BSP_morse_signal(uint8_t onoff);
void BSP_stop_everything(void);
void BSP_do_reset(void);

uint8_t BSP_button_1(void);
uint8_t BSP_button_2(void);
uint8_t BSP_button_3(void);

void BSP_start_temperature_reading(void);
void BSP_get_temperature(void);

int16_t BSP_get_calibration(void);
void BSP_save_calibration(int16_t cal);
int16_t BSP_get_adjustment(void);
void BSP_save_adjustment(int16_t cal);

#ifdef __MSP430__
#define BSP_delay_ms(ms) __delay_cycles(1000L*ms)
#endif


#endif

