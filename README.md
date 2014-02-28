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
  * + and - signs.
  * 4 arrow signs.
 * Many more display components than I originally wanted, but I may just
   have to increase functionality to match.
 * Available from http://microcontrollershop.com/
 * I have five.

## Temperature sensor

 * MCP9700A
  * Analogue output
  * Vcc down to 2.3V

# Power

 * 2 x AAA cells.

