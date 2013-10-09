#ifndef bsp_449STK2_h_INCLUDED
#define bsp_449STK2_h_INCLUDED

/**
 * @file Stuff specific to the MSP430-449STK2 board.
 */

#include <msp430.h>

/** Enter the appropriate MSP430 low power mode.  We must also enable
    interrupts.  We can't use LPM4 as that stops all clocks so the basic timer
    doesn't work. */
#define ENTER_LPM() _BIS_SR(GIE + LPM3_bits)


/** Exit the MSP430 low power mode.  This must match the bits in
    ENTER_LPM(). */
#define EXIT_LPM() _BIC_SR_IRQ(GIE + LPM3_bits)

#endif
