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

enum adc_channel {
	temperature_channel = 1,
	vcc_channel = 2,
};

uint8_t BSP_start_temperature_reading(QActive *ao, QSignal signal,
				      enum adc_channel channel);
void BSP_get_temperature(void);
int16_t BSP_convert_adc_to_temperature(uint16_t adc);

int16_t BSP_get_calibration(void);
void BSP_save_calibration(int16_t cal);
int16_t BSP_get_adjustment(void);
void BSP_save_adjustment(int16_t cal);

#ifdef __MSP430__
#define BSP_delay_ms(ms) __delay_cycles(1000L*ms)
#endif

void BSP_restart_seconds(void);
void BSP_add_8th_second(void);
void BSP_sub_8th_second(void);

#endif

