/*
 * hardwareserial.c - functions for SAMD10 USART
 *
 * Created: 9/1/2015 10:46:35 PM
 *  Author: Bill Westfield
 */ 
#include "Arduino.h"
#include <ASF.h>

#define RXPIN 0
#define TXPIN 1

//-----------------------------------------------------------------------------
#define uart_sync(a)

void uart_init_ (int baud_div)
{
	PortGroup *const port_base = &(PORT->Group[0]); /* port_get_group_from_gpio_pin(1); */ // any pin will work.
	
	port_base->PINCFG[digital_pin_to_pinno[TXPIN]].reg = PORT_PINCFG_PMUXEN;
	port_base->PINCFG[digital_pin_to_pinno[RXPIN]].reg = PORT_PINCFG_PMUXEN;
	port_base->PMUX[digital_pin_to_pinno[RXPIN]/2].reg = 0x22;  // Periph function "C" - SERCOM0

	PM->APBCMASK.reg |= PM_APBCMASK_SERCOM0;  // enable power

	GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID(SERCOM0_GCLK_ID_CORE) |
	GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN(0);   // provide clock

	SERCOM0->USART.CTRLA.reg =
	SERCOM_USART_CTRLA_DORD | SERCOM_USART_CTRLA_MODE_USART_INT_CLK |
	SERCOM_USART_CTRLA_RXPO(3/*PAD3*/) | SERCOM_USART_CTRLA_TXPO(1)/*PAD2*/;
	
	SERCOM0->USART.CTRLB.reg = SERCOM_USART_CTRLB_RXEN | SERCOM_USART_CTRLB_TXEN |
	SERCOM_USART_CTRLB_CHSIZE(0/*8 bits*/);

	SERCOM0->USART.BAUD.reg = baud_div;

	SERCOM0->USART.CTRLA.reg |= SERCOM_USART_CTRLA_ENABLE;
}

//-----------------------------------------------------------------------------
void uart_putc(char c)
{
	while (!(SERCOM0->USART.INTFLAG.reg & SERCOM_USART_INTFLAG_DRE))
	;
	SERCOM0->USART.DATA.reg = c;
}

int uart_getc(void)
{
	if (SERCOM0->USART.INTFLAG.reg & SERCOM_USART_INTFLAG_RXC) {
		return SERCOM0->USART.DATA.reg;
	}
	return -1;
}

//-----------------------------------------------------------------------------
void uart_puts(const char *s)
{
	while (*s)
	uart_putc(*s++);
}

