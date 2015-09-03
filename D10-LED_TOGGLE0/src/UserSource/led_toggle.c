/**
 *
 */

#include <asf.h>
#include "clock.h"
#include "Arduino.h"

#define CUSTOMCLOCK 1

#define USEPLL 1
	#define EXTCLK_FREQ 8000000  /* External clock freq on SAMD10 Xplained mini */
	#define F_INTERM 32000		 /* Intermediate frequency.  <33KHz for FLL, <2MHz for PLL */

#ifdef CUSTOMCLOCK
void system_clock_init()
{
#ifdef USEPLL
#define EXTCLK_DIV ((EXTCLK_FREQ/(2*F_INTERM)) - 1)  // Slightly complicated
	//Enable the 8MHz oscillator input from SAMD10 XMini
	SYSCTRL->XOSC.reg = (SYSCTRL_XOSC_ENABLE);
	
	//Wait for the crystal oscillator to start up
	while((SYSCTRL->PCLKSR.reg & (SYSCTRL_PCLKSR_XOSCRDY)) == 0);
		
	//Set flash wait state to 1, which we need to do at 48MHz
	REG_NVMCTRL_CTRLB |= (NVMCTRL_CTRLB_RWS(1));
	
	//Configure the DPLL

	SYSCTRL->DPLLCTRLA.reg = 0;    // Defaults!

	/*
	 * The output of DPLL is supposed to be between 48MHz and 96MHz.
	 * By setting the PLL output to 2*F_CPU, we have easy acess to
	 * most of the "interesting" frequencies between 24MHz and 48MHz
	 * Lower F_CPU can be obtained by increasing the GCLK0 divisor.
	 */
	SYSCTRL->DPLLRATIO.reg = SYSCTRL_DPLLRATIO_LDR(2*F_CPU/F_INTERM);
	SYSCTRL->DPLLCTRLB.reg = SYSCTRL_DPLLCTRLB_DIV(EXTCLK_DIV) | /* Refclock divider */
		SYSCTRL_DPLLCTRLB_REFCLK(SYSTEM_CLOCK_SOURCE_DPLL_REFERENCE_CLOCK_XOSC);
	SYSCTRL->DPLLCTRLA.reg = SYSCTRL_DPLLCTRLA_ENABLE;
#if 0
	REG_SYSCTRL_DPLLRATIO  = SYSCTRL_DPLLRATIO_LDR(2*F_CPU/F_INTERM);
	REG_SYSCTRL_DPLLCTRLB = SYSCTRL_DPLLCTRLB_DIV(EXTCLK_DIV) | /* Refclock divider */
		SYSCTRL_DPLLCTRLB_REFCLK(SYSTEM_CLOCK_SOURCE_DPLL_REFERENCE_CLOCK_XOSC);
	REG_SYSCTRL_DPLLCTRLA = SYSCTRL_DPLLCTRLA_ENABLE;
#endif
	while(!(SYSCTRL->DPLLSTATUS.reg & (SYSCTRL_DPLLSTATUS_CLKRDY | SYSCTRL_DPLLSTATUS_LOCK)) ==
									  (SYSCTRL_DPLLSTATUS_CLKRDY | SYSCTRL_DPLLSTATUS_LOCK));
	
	//For generic clock generator 0, select the DPLL Clock as input, divide by 2
	GCLK->GENDIV.reg = ((2 << GCLK_GENDIV_DIV_Pos) | (0 << GCLK_GENDIV_ID_Pos));
	GCLK->GENCTRL.reg = ((0 << GCLK_GENCTRL_ID_Pos) | (GCLK_SOURCE_FDPLL << GCLK_GENCTRL_SRC_Pos)| (GCLK_GENCTRL_GENEN));
	GCLK->CLKCTRL.reg = ((GCLK_CLKCTRL_GEN_GCLK0) | (GCLK_CLKCTRL_CLKEN)) ;
#endif /* USEPLL */
#ifdef USEFLL
	Assert(F_CPU == 48000000);  /* FLL Output must be 48MHz */
	Assert(F_INTERM < 33000);   /*  FLL input must be less than 33 kHz */
#define EXTCLK_DIV (EXTCLK_FREQ/F_INTERM)
	//Enable the 8MHz oscillator input
	SYSCTRL->XOSC.reg = (SYSCTRL_XOSC_ENABLE);
	
	//Wait for the crystal oscillator to start up
	while((SYSCTRL->PCLKSR.reg & (SYSCTRL_PCLKSR_XOSCRDY)) == 0);
	
	//Configure GCLK Generator 1 to use a divided external osc as input, and feed this clock to the DFLL48M FLL
	GCLK->GENDIV.reg = ((EXTCLK_DIV << GCLK_GENDIV_DIV_Pos) | (1 << GCLK_GENDIV_ID_Pos));
	GCLK->GENCTRL.reg = ((1 << GCLK_GENCTRL_ID_Pos) | (GCLK_GENCTRL_SRC_XOSC) | (GCLK_GENCTRL_GENEN));
	GCLK->CLKCTRL.reg = ((GCLK_CLKCTRL_GEN_GCLK1) | (GCLK_CLKCTRL_CLKEN) | (GCLK_CLKCTRL_ID(0))) ;
	
	//Configure the FDLL48MHz FLL, we will use this to provide a clock to the CPU
	//Set the course and fine step sizes, these should be less than 50% of the values used for the course and fine values (P150)
	SYSCTRL->DFLLCTRL.reg = (SYSCTRL_DFLLCTRL_ENABLE); //Enable the DFLL
	SYSCTRL->DFLLMUL.reg = (SYSCTRL_DFLLMUL_CSTEP(5) | SYSCTRL_DFLLMUL_FSTEP(10));
	SYSCTRL->DFLLMUL.reg |= (SYSCTRL_DFLLMUL_MUL(F_CPU/(EXTCLK_FREQ/EXTCLK_DIV)));
	SYSCTRL->DFLLCTRL.reg |= (SYSCTRL_DFLLCTRL_MODE);
	
	//Wait and see if the DFLL output is good . . .
	while((SYSCTRL->PCLKSR.reg & (SYSCTRL_PCLKSR_DFLLLCKC)) == 0);
	
	//Set flash wait state to 1, which we need to do at 48MHz
	REG_NVMCTRL_CTRLB |= (NVMCTRL_CTRLB_RWS(1));
	
	//For generic clock generator 0, select the DFLL48 Clock as input
	GCLK->GENDIV.reg = ((1 << GCLK_GENDIV_DIV_Pos) | (0 << GCLK_GENDIV_ID_Pos));
	GCLK->GENCTRL.reg = ((0 << GCLK_GENCTRL_ID_Pos) | (GCLK_GENCTRL_SRC_DFLL48M) | (GCLK_GENCTRL_GENEN));
	GCLK->CLKCTRL.reg = ((GCLK_CLKCTRL_GEN_GCLK0) | (GCLK_CLKCTRL_CLKEN)) ;
#endif

}
#endif /* CUSTOMCLOCK */

#define CpuCriticalVar()  uint8_t cpuSR
 
#define CpuEnterCritical()              \
  do {                                  \
    asm (                               \
    "MRS   R0, PRIMASK\n\t"             \
    "CPSID I\n\t"                       \
    "STRB R0, %[output]"                \
    : [output] "=m" (cpuSR) :: "r0");   \
  } while(0)
 
#define CpuExitCritical()               \
  do{                                   \
    asm (                               \
    "ldrb r0, %[input]\n\t"             \
    "msr PRIMASK,r0;\n\t"               \
    ::[input] "m" (cpuSR) : "r0");      \
  } while(0)
  
/** Handler for the device SysTick module, called when the SysTick counter
 *  reaches the set period.
 *
 *  \note As this is a raw device interrupt, the function name is significant
 *        and must not be altered to ensure it is hooked into the device's
 *        vector table.
 */
static uint8_t ledval = 0;


static volatile uint32_t rtcount;
uint32_t testmicros[100];

int main(void)
{
	system_init();
	uint32_t nextprint = 0;
	int c;

	RTC_init();  /* Initialized RTC to provide us count and ms interrupts. */
	uart_init(57600);
	
	pinMode(13, OUTPUT);
	delay(2000);
	uart_puts("\nthis is a test \n");
	
	for (int i=0; i<100; i++) {
		delayMicroseconds(50);
		testmicros[i]= micros();
	}
	while (true) {
//		rtcount = RTC_get_count();
		delay(5);
		digitalWrite(13, 1);
		delay(5);
		digitalWrite(13, 0);
		if (millis() > nextprint) {
			uart_puts("\nA couple seconds have done by");
			if ((c = uart_getc()) > 0) {
				uart_puts(" And I received a '");
				uart_putc(c);
				uart_putc('\'');
			}
			nextprint += 2000;
		}
	}
}
