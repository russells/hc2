# "Hots and Colds thermometer."

Displays temperature as

 * 20C = "NORMAL"
 * < 20C = "NN COLDS", where NN = 20-T
 * \> 20C = "NN HOTS", where NN = T-20

A wedding present for Andrew and Kate.

# Display requirements

 * Really need an eight digit display but can do with seven.
 * "12 Colds" is eight digits.

# Hardware

## Microcontroller

 * MSP430F449IPZ.
 * Has a builtin LCD controller.
 * http://www.ti.com/product/msp430f449
 * Available from RS.

## Display

One of these types will be used.  Some of each are on the way.

### Varitronix VIM-828

 * 14 segment starburst, 8 digits, with 7 decimal points.
 * Made by varitronix, but not available from them as a retail component.
 * Not easily available from very many places at all.

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

 * I have some MCP9700A devices.
  * Analogue output
  * Vcc down to 2.3V

# Power

 * 2 x AA cells.

