# SAMD10-experiments

## Note: This is a work-in-progress

Working: pinMode, digitalWrite, Serial primitives (polled), msclock, mills, micros, delay, delayMicroseconds, analogWrite using TCC0.
In Progress: analogWrite using TC0/1, digitalRead

This is an experiment in adding arduino-core-like functionality to the Atmel SAMD10 Xplained Mini Eval board.
Because implementing the Ardunio core is a good way to explore the capabilities of a microcontroller.

This started out as the SAMD10-LED-TOGGLE example project (In Atmmel Studio 6.2), which has gradually had references to ASF functions removed, and functionality added.  It was imported, so it has a great deal of unused source files in the ASF directory.  ASF files come under Atmel's license and copyright.  The new code is licensed MIT-style.

Since the SAMD10 family has relatively modest amounts of memory, and since Atmel's ASF library are rather bloated, this code does not use ASF; it uses direct manipulation of the bare registers.  (Note that since the Atmel CMSIS chip and peripheral definitions are considered part of ASF, there is still an ASF directory in the project.)

### Notes on Clock Initialization
My clock function has inputs of F_CPU (desired clock rate), the External Clock rate, and the desied intermediate freq (32kHz) instead of clock sources and divisors.

SAMD10 Xplained Mini routes an 8MHz clock from the debug circuitry to the D10 Xin pin; I use this as the reference.

We can derive a faster clock from that pin using either the DFLL or the DPLL.   The DFLL only supports an output of 48MHz, while the DPLL supports an output between 48 and 96MHz, further divided by 2 (to meet the CPU max frequency specs) (or more.)  This means that the DPLL is much more flexible; essentially capable of producing any interesting frequency.

It's not clear why one would use the DFLL.
http://www.avrfreaks.net/forum/samd10-xplained-mini-questions-and-experiences-clocking-transitioning-pinmux

Inputs to both DFLL and DPLL need to be ~32kHz.  For using the 8MHz input (or 8MHz internal clock), it is first divided by <bignumber> to get 32kHz.

### General notes on peripherals
A fair number of peripherals are turned on in the power manager by default, but not all of them.  If you enable a peripheral clock, but are unable to enabled the peripheral itself, that probably means that the peripheral is not enabled in the power manager.

Don't forget to set the PMUXEN bit the the port config to enable the mux set in the PINMUX register.

There are constants defined in ASF of the form PINMUX_PAnnv_periph_func (ie PINMUX_PA09E_TCC0_WO3) that have the pinmux possibilities for pin PAnn.  nn in the high 16 bits, v (the pinmux value) in the low 16 bits.


### Notes on GPIO
Shield pin 6 (PA30) has the SW Debug traffic running on it, so you can't use it while debugging.

SET/CLEAR/TOGGLE registers are the quickest way to manipulate single bits.  SAMDxx does not seem to have bit-banding.

The fact that an ARM can shift by any number of bits in a single cycle can have a significant impact on code design.  I map "Shield pin numbers" to bit positions rather than bit masks, for example.

SAMD10 only has a single ("32bit") GPIO port.   The chip has up to 24 pins, and the gpio bits are scattered somewhat randomly over the pins.  http://www.avrfreaks.net/forum/samd10-pinout-venting

There's a table here: https://docs.google.com/spreadsheets/d/1y13QMuydCw7TpIcOEO_Sfz02DZC6AI7C76_Tfo7ayag/edit?usp=sharing
And something for pinmux here: https://docs.google.com/spreadsheets/d/12YnZpTPGxS-oINhrRfFgx2rkwPUyUYQbXvVIME8oCHY/edit#gid=0

An AVR can set a constant GPIO pin to a constant value in a single 2-clock instruction, but an ARM takes 4 instructions to accomplish the same thing.  On the other hand, fully implementing a variable pin/value with mapping (as in the Arudino digitalWrite() function) takes about a dozen instructions, compared to about 50 for AVR. 

There's a high speed IOBUS for accessing the GPIO Ports at 0x60000000; it's not well documented.

The call setup to call digitalWrite() is typically 3 instructions, making inlining the function when used with constant arguments pretty compelling.

http://www.avrfreaks.net/comment/1622676#comment-1622676

"Open Drain" is mentioned as possible in the datasheet, but doesn't seem to be implemented.


### Notes on SysTick
It's common to use the ARM SysTick Timer as the main periodic clock (ie for Arduino millis()/etc), but the SAMD10 RTC is more flexible.

The SysTick timer is 24 bits.  This means that if you increase the system clock to 48MHz, you can no longer use it to produce a 1s interrupt (as in the LED_TOGGLE example !)


### Notes on RTC
The RTC has several modes, and some of them are similar in function to the ARM "Systick" clock, but more configurable.  Here, we set the RTC to count microseconds, and interrupt every millisecond.

The RTC does not reset on system RESET.  Only at poweron or explicit RTC Reset command.  So it MUST be disabled before it can be initialized.

Clock synchronization is complicated, causes weird behavior, and is slow.  If a peripheral (like the RTC) has a prescaler that is part of the peripheral, it is better to initialize the peripheral with a fast GCLK and divide using the prescaler, than it is to use a slower divided GCLK without the peripheral prescaler (because synchronization depends on the GCLK provided.)

In this case, we set up the 96MHz core clock, divided by 6 to yield a 16MHz RTC clock input, and then use the RTC prescaler to divide that down to 1MHz.

 Note that the RTC is different across different SAM parts - on some parts (SAMC and SAMD5x in particular), the RTC can only be clocked by a dedicated 32kHz oscillator, while on SAMD10 and SAMD21 it can be clocked by any GCLK.  (presumably a dedicated low-frequency clock allows for lower-power sleep modes.)

http://www.avrfreaks.net/forum/samd10-rtc


### Notes on UART
http://www.avrfreaks.net/forum/samd21-serial-port-setup-and-hello-world
The mEDBG USB/Serial port seems to be limitted to 57600bps.

The USART "PAD" configuration is confusing.  You use the GPIO pinmux configuration to route certain SERCOM "PADS" to certain pins, and then SERCOM configuration to route certain serial functions to the (internal) PADs.  Here, we only use RXD and TXD.

The current UART implementation is polled.

The UART seems to have a 2-byte FIFO.  If you send it N bytes while not paying attention, you'll end up reading the 1st, 2nd, and last (from the shift register) bytes that were sent.

The Baud Rate Generator seems a little unusual, and looks like it requires a 64-bit division to cover all baud rates.  This is very expensive on CM0 (sucks in big library code) if you can't get it to happen at compile-time instead.

The BRG has two async modes; one with an "integer divisor", and one with a "fractional divisor."   You might think that the fractional divisor would be more complicated, but it's not.   The Integral divisor is "weird" and typical code will use 64bit math to compute the BRG register contents.   The fractional divisor uses simpler math, and the faction will be zero for most common bitrates with a 48MHz clock.

BRG calcuations can be done at compile-time, of course, if the baud rate is a constant.  Which is good, because otherwise you suck in division code (no divide instruction on M0)

Atmel put the fractional part of the BRG register in the wrong place.  Sigh.

The SAMD0 has three "SERCOM" units.  Normally one will be dedicated to UART, one to SPI, and one to I2C.

### Notes on Timers
Two kinds of timers: TCC0 (for Control) has 4 compare channels (8 outpus, some of which would have to complemented to be useful), and TC1/2 have 2 compare channels each (2 outputs.)

Despite fancy multiplexors and 8 total compare channels, if you want to route all the compare channels to pins, you don't have many choices.


### Notes on ASF
While this project is supposed to end up NOT using ASF, looking at the existing ASF code to figure out how it does things is frequently useful.

Atmel apparently considers CMSIS defs to be part of ASF, so the CMSIS files and definitions appear in the ASF part of the project tree.

Peripherals are defined as nice CMSIS-style structures in cmsis/samd10/include/component/\<periph\>.h and the samd10d14am.h files, with individual register addresses defined in cmsis/samd10/include/instance/\<periph\>.h  Using the structures when accessing multiple peripheral registers is faster, because the base-address only needs to be loaded once.

Peripheral registers in ASF generally include a bottom-level union of ".reg" (full-width access to the register) and ".bits.fieldname" allowing access of individual bitfields (note that the .bit.xxx names are bitFIELDS and not (always) individual bits.)  The .reg files will let you set multiple fields at one time.
Note that the CM0+ does not have bitfield instructions, so accessing bitfields is not very efficient.

There are a bunch of defined symbols that seem to have an obvious function, but they are actually more complicated ASF "abstractions."  The PINMUX symbols mentioned above are one example.

PORT_PAnn is a bitmask (bit nn)  PIN_PAnn is a bit number.

Be careful with using the bitfield structures with the numerous xxxCLR and xxxSET io registers.  The compiler will produced load/clearbits/setbits/store sequences, which will frequently do the wrong thing, especially with the xxxCLR registers.  For example, "SERCOM5->USART.INTENCLR.bit.DRE = 1;" will end up clearing any other interrupts that were enabled at the time, since they read as one, and clear the bit when the one is written.  https://community.atmel.com/forum/problem-clearingsetting-bit-interrupt-flag-register


### Notes on Floating Point
The Cortex-M0+ lacks floating point hardware, of course.  However, it also lacks "chip-optimized" floating point functions in libgcc.  If you use floating point on a CM0 or CM0+ chip, you will get the "generic" libgcc floating point code, written in C, that is quite large and slow, even compared with something like a CM3 (which DOES have chip-optimized float code.)  This is ... annoying, since the CM0+ has less memory and slower clock.  (however, see: https://www.quinapalus.com/qfplib.html )

----

AVFreaks users "kernels" and "alexru" have been especially helpful!
