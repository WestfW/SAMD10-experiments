/*
 * Wiring_digital.c
 * Arduino digital pin Functions for SAMD10
 * Copyright 2015 by William Westfield
 */
#include "Arduino.h"
#include <asf.h>

/*
 * Map Arduino pin numbers to hardware specific values.
 * In the case of the D10, all the pins bits in a single GPIO port
 * so it's efficient to map to bit numbers (which an ARM can easily
 * convert to a bitmask, since it has a barrel shifter.)
 * -1 means the digital pin doesn't exist.
 */

 const int8_t digital_pin_to_pinno[] = {
  11, 10, 16, 17,
  27, 25, 30, 31,
  -1, -1, 23, 22,
  24,  9, 02, 03,
  04, 05, 06, 07,
  15, 14
};

/*
 * digitalWrite(pinno, level)
 * This is logically similar to the ASF function port_pin_set_output_level(),
 * but uses a different mechanism for mapping the "pinno" to the port/bit.
 */

void digitalWrite(uint8_t pinno, uint8_t level)
{
  PortGroup *const port_base = &(PORT->Group[0]); /* port_get_group_from_gpio_pin(1); */ // any pin will work.
  uint32_t bitno  = digital_pin_to_pinno[pinno];

  port_base->PINCFG[digital_pin_to_pinno[pinno]].reg &= ~PORT_PINCFG_PMUXEN;
  if (level) {
    port_base->OUTSET.reg = 1<<bitno;
  } else {
    port_base->OUTCLR.reg = 1<<bitno;
  }
}

void pinMode(uint8_t pinno, uint8_t direction)
{
	PortGroup *const port_base = &(PORT->Group[0]); /* port_get_group_from_gpio_pin(1); */ // any pin will work.
	uint32_t pin_mask  = (1UL << (digital_pin_to_pinno[pinno]));
	
	if (direction == OUTPUT) {
		port_base->DIRSET.reg = pin_mask;
		} else {
		port_base->DIRCLR.reg = pin_mask;
	}
}
