# SAMD10-experiments

## Note: This is a work-in-progress

This is an experiment in adding arduino-core-like functionality to the Atmel SAMD10 Xplained Mini Eval board.
Because implementing the Ardunio core is a good way to explore the capabilities of a microcontroller.

This started out as the SAMD10-LED-TOGGLE example project (In Atmmel Studio 6.2), which has gradually had references to ASF functions removed, and functionality added.  It was imported, so it has a great deal of unused source files in the ASF directory.  ASF files come under Atmel's license and copyright.  The new code is licensed MIT-style.

Since the SAMD10 family has relatively modest amounts of memory, and since Atmel's ASF library are rather bloated, this code does not use ASF; it uses direct manipulation of the bare registers.  (Note that since the Atmel CMSIS chip and peripheral definitions are considered part of ASF, there is still an ASF directory in the project.)

### Notes on Clock initialization
SAMD10 Xplained Mini routes an 8MHz clock from the debug circuitry to the D10 Xin pin.

We can derive a faster clock from that pin using either the DFLL or the DPLL.   The DFLL only supports an output of 48MHz, while the DPLL supports an output between 48 and 96MHz, further divided by 2 (to meet the CPU max frequency specs.)  This means that the DPLL is much more flexible; essentially capable of producing any interesting frequency above 8MHz.

It's not clear why one would use the DFLL.
http://www.avrfreaks.net/forum/samd10-xplained-mini-questions-and-experiences-clocking-transitioning-pinmux

### Notes on GPIO
SAMD10 only has a single GPIO port.   It has up to 24 pins, and those are scattered somewhat randomly over the 32bit registers that manipulate the port.  http://www.avrfreaks.net/forum/samd10-pinout-venting

There's a table here: https://docs.google.com/spreadsheets/d/1y13QMuydCw7TpIcOEO_Sfz02DZC6AI7C76_Tfo7ayag/edit?usp=sharing

An AVR can set a constant GPIO pin to a constant value in a single 2-clock instruction, but an ARM takes 4 instructions to accomplish the same thing.  On the other hand, fully implementing a variable pin/value with mapping (as in the Arudino digitalWrite() function) take about a dozen instructions, compared to about 50 fo AVR. 

http://www.avrfreaks.net/comment/1622676#comment-1622676

"Open Drain" is mentioned as possible in the datasheet, but doesn't seem to be implemented.

### Notes on RTC
The RTC has several modes, and some of them are similar in function to the ARM "Systick" clock, but more configurabel.  Here, we set the RTC to count microseconds, and interrupt every millisecond.

Clock synchronization is complicated, causes weird behavior, and is slow.  If a peripheral (like the RTC) has a prescaler that is part of the peripheral, it is better to initialize the peripheral with a fast GCLK and devide using the prescaler, than it is to use a slower divided GCLK without the peripheral prescaler.

In this case, we set up the 96MHz core clock, divided by 6 to yield a 16MHz RTC clock input, and then use the RTC prescaler to divide that down to 1MHz.

http://www.avrfreaks.net/forum/samd10-rtc

### Notes on UART
http://www.avrfreaks.net/forum/samd21-serial-port-setup-and-hello-world
The mEDBG USB/Serial port seems to be limitted to 57600bps.

The USART "PAD" configuration is confusing.  You use the GPIO pinmux configuration to route certain SERCOM "PADS" to certain pins, and then SERCOM configuration to route certain serial functions to the (internal) PADs.  Here, we only use RXD and TXD.

The current UART implementation is polled.

The UART seems to have a 2-byte FIFO.  If you send it N bytes while not paying attention, you'll end up reading the 1st, 2nd, and last (from the shift register) bytes that were sent.

### Notes on Timers

### Notes on ASF
While this project is supposed to end up NOT using ASF, looking at the existing ASF code to figure out how it does things is frequently useful.


----

AVFreaks users "kernels" and "alexru" have been especially helpful!
