/*
 * wiring_analog.c
 *
 * Created: 9/5/2015 8:51:06 PM
 * Copyright 2015 by Bill Westfield
 */ 
#include "Arduino.h"
#include <asf.h>

#define TC1_C(n) ((volatile uint8_t *)&(TC1->COUNT8.CC[n].reg))
#define TC2_C(n) ((volatile uint8_t *)&(TC2->COUNT8.CC[n].reg))
#define TCC_C(n) ((volatile uint8_t *)&(TCC0->CC[n].reg))

volatile uint8_t* const pin2pwmcount[] =
{0,      0,        TCC_C(2), TC1_C(1),
	0,   TCC_C(3), TC2_C(0),    0,
	0,   TC1_C(1), TCC_C(1), TCC_C(0),
	0,   TCC_C(3)
};

static int maxpwmpinno = (sizeof(pin2pwmcount)/sizeof(pin2pwmcount[0]));


void analogWrite_init()
{
	PM->APBCMASK.reg |= PM_APBCMASK_TCC0;  // enable power
	GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID(TCC0_GCLK_ID) |    // provide clock
	GCLK_CLKCTRL_GEN(0 /* MYCLK_16MHZ */) | GCLK_CLKCTRL_CLKEN;
	TCC0->CTRLA.reg = TCC_CTRLA_CPTEN3 | TCC_CTRLA_PRESCALER_DIV64 |
	TCC_CTRLA_ENABLE;
	TCC0->WAVE.reg = TCC_WAVE_WAVEGEN_NPWM; // Normal PWM
	TCC0->PER.reg = 255;  // 8bit mode
	TCC0->CTRLA.reg = TCC_CTRLA_CPTEN3 | TCC_CTRLA_PRESCALER_DIV64 |
	TCC_CTRLA_ENABLE;

	PORT->Group[0].PMUX[ 9/2].bit.PMUXO = PINMUX_PA09E_TCC0_WO3 & 0xFFFF;  // pin 13 to timer.
	PORT->Group[0].PMUX[25/2].bit.PMUXO = PINMUX_PA25E_TCC0_WO3 & 0xFFFF;  // pin  5 to timer.
	PORT->Group[0].PMUX[30/2].bit.PMUXE = PINMUX_PA30E_TC2_WO0 & 0xFFFF;  // pin  6 to timer.
	PORT->Group[0].PMUX[15/2].bit.PMUXO = PINMUX_PA15E_TC1_WO1 & 0xFFFF;  // pin (scl) to timer.
	PORT->Group[0].PMUX[22/2].bit.PMUXE = PINMUX_PA22F_TCC0_WO4 & 0xFFFF;  // pin 11 to timer.
	PORT->Group[0].PMUX[17/2].bit.PMUXO = PINMUX_PA17E_TC1_WO1 & 0xFFFF; // pin 3 to TC1
	PORT->Group[0].PMUX[23/2].bit.PMUXO = PINMUX_PA23F_TCC0_WO5 & 0xFFFF;  // pin 13 to timer.
	PORT->Group[0].PMUX[16/2].bit.PMUXE = PINMUX_PA16F_TCC0_WO6 & 0xFFFF;  // pin 2 to timer.
}


void analogWrite(int pin, int val)
{
	PortGroup *const port_base = &(PORT->Group[0]); /* port_get_group_from_gpio_pin(1); */ // any pin will work.
	int bitno = digital_pin_to_pinno[pin];
	
	port_base->PINCFG[bitno].reg |= PORT_PINCFG_PMUXEN;
	*(pin2pwmcount[pin]) = val;
}
