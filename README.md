# "Hots and Colds thermometer."

Displays temperature as

 * 20C = "NORMAL"
 * < 20C = "NN COLDS", where NN = 20-T
 * \> 20C = "NN HOTS", where NN = T-20

A wedding present for Andrew and Kate.

# Hardware

## Microcontroller

 * MSP430F449IPZ.
 * Has a builtin LCD controller.
 * http://www.ti.com/product/msp430f449
 * Available from RS.

## Display

### SoftBaugh SBLCDA2

 * Has many display components
    * 14 segment starburst, 7 digits, with 6 decimal points.
    * 7 segment, 4 digits, with 3 decimal points, and colon.
    * Battery meter.
    * Progress bar.
    * Antenna and 3 part signal strength meter.
    * Plus and minus signs.
    * 4 arrow signs.
 * Because of the extra components, more than just having a set of 7 or 14
   segment characters, we can implement a clock and a menu system.
 * Available from http://microcontrollershop.com/

## Temperature sensor

 * MCP9700A
    * Analogue output
    * Vcc down to 2.3V

## Board

 * Made at PCB Zone, http://pcbzone.net/

# Power

 * 2 x AAA cells.

## Power measurements

 * With serial and LED disabled, power consumption has been measured at
   25.8uA mean.
    * 13uA background
    * 60uA, 50us pulses, 3.9ms apart - this is the LCD clock.
    * 430uA, 170 us pulses, at 32Hz (31250us period) - this is the timer
      tick with short periods of 1MHz clock.
    * 510uA, 38ms pulses, at 0.5Hz (2000ms period) - this is the ADC being
      read every two seconds, along with the extra CPU and 1MHz clock
      activity.
    * Total power = 13 + 60\*(50/3900) + 430\*(170/31250) + 510\*(38/2000)
      = 25.8uA
 * With the batteries at 2.9V, one battery was removed.  The unit ran for
   30 minutes, with the capacitor supply voltage dropping to 2.25V.
