# Improved 1-Wire stack for DS2485 -- *1-Wire bus* -- DS28E18

## Background
I'm using an
[ENS210 high-precision temperature and humidity sensor](https://www.sciosense.com/ens21x-family-of-high-performance-digital-temperature-and-humidity-sensors/)
at the end of a long 1-Wire cable. 
The 1-Wire bus is driven at the host MCU with a DS2485, and at the remote end a DS28E18 controls and powers the ENS210.

I had some difficulty using the software provided by Maxim (now Analog Devices) in one of our projects.
Ultimately, I ended up reworking it considerably to get it into shape we could use.
Hope this resulting code helps other folks trying to use 1-Wire!

Platform dependencies are isolated, so its easy to move to a different platform.
This example uses FreeRTOS and NXP FSL LPI2C.

## Modified 1-Wire Stack
The 1wire directory contains the 1-wire stack based on Maxim-provided code, 
with modifications listed below to be usable in a real application.
Also adds a platform-specific driver for NXP RT1024 using FSL and FreeRTOS,
with both DMA (performant) and polling (slow but simpler) implementations.

** Note: ** This is not an Arduino driver.
However, as the OS dependencies are now isolated in one source module,
this stack can easily be adapted to Arduino, Windows, whatever
(just create an appropriate DS2485_port_xxx.c).

## Example ENS210 Driver
The C++ driver code for the ENS210 is included here along with the 1-Wire stack.

## Maxim code modification summary:
* DS2485 refactored for platform independence (isolated low-level platform-specific I2C API)
* code cleanup (especially DS28E18), resulting in smaller more maintainable source code, less flash use, and faster runtime:
  * fix compile errors
  * unnecessary includes removed
  * use standard modern types (uint8_t rather than u_char, etc.)
  * variable naming greatly improved
  * replaced some grossly inefficient coding
  * massive cut-and-paste factored out
  * use static for local functions and variables
  * move non-public (non-API) definitions out of API header files
  * correct function types to match types actually returned (bool confusion)
  * commentary and Doxygen output expanded and corrected (could still use a lot of improvement)
  * in APIs, use const when passing read-only arrays (enables constant ie flash-resident data to be passed)
  * use static const for constant parameter arrays (save code space and runtime)
  * DS28E18 especially: API rationalized
* fixed a few bugs
* added platform specialization for iMXRT1024 in DS2485_port_NXP_LPI2C.c

## Pre-Built DOxygen Documentation
I've included the complete DOxygen but can't figure out how to get GitHub to let you look at it
(this only renders the index page). 
If you know how to make this work let me know, Thanks!

https://htmlpreview.github.io/?https://github.com/DRNadler/1Wire/blob/main/1wire_and_ENS210_doxygen/html/index.html

## Reference
[Original source from Maxim is in MAXREFDES9004.ZIP file on this DS28E18 product page](https://www.analog.com/en/products/ds28e18.html#product-tools).
Download MAXREFDES9004: C-source Reference for Operating the DS2485 Combined with a DS28E18 from a Cortex-M4 Microcontroller
