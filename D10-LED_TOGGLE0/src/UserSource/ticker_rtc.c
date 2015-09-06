/*
 * ticker.c
 * Periodic clock tick and timing functions for SAMD10.
 * Copyright 2015 by William E Westfield
 */
#include "Arduino.h"
#include "clock.h"

#define DEBUG 1

volatile uint32_t millisecondcount = 0;

#if DEBUG
volatile uint32_t rtctick_lastmicros;
volatile uint32_t rtctick_backwards = 0;
volatile uint32_t rtctick_microscalled = 0;

typedef struct {unsigned long current, last, count;} trace_t;
trace_t rtctick_trace[8];
int rtctick_tracei = 0;

#endif

#define CpuCriticalVar()  uint8_t cpuSR

#define CpuEnterCritical()			\
  do {						\
    asm (					\
	 "MRS   R0, PRIMASK\n\t"		\
	 "CPSID I\n\t"				\
	 "STRB R0, %[output]"			\
	 : [output] "=m" (cpuSR) :: "r0");	\
  } while(0)

#define CpuExitCritical()			\
  do{						\
    asm (					\
	 "ldrb r0, %[input]\n\t"		\
	 "msr PRIMASK,r0;\n\t"			\
	 ::[input] "m" (cpuSR) : "r0");		\
  } while(0)

void RTC_init()
{
  GCLK->CLKCTRL.reg = ((GCLK_CLKCTRL_GEN_GCLK2) | (GCLK_CLKCTRL_CLKEN) |
		       (GCLK_CLKCTRL_ID(RTC_GCLK_ID)));
  // Now configure the RTC preacaler for divide-by-16, for 1MHz "tick", and have it
  //   count to 1000.  Microsecond ticks, millisecond interrupts.
  // Note that using a higher input clock and larger prescaler divider cuts down on
  //  clock synchronization issues that are otherwise really annoying.
  RTC->MODE0.CTRL.reg = RTC_MODE0_CTRL_SWRST; //reset
  while (RTC->MODE0.CTRL.bit.SWRST)
    ;  // wait for reset complete
  RTC->MODE0.CTRL.reg = RTC_MODE0_CTRL_MODE_COUNT32;
  RTC->MODE0.CTRL.reg |= RTC_MODE0_CTRL_MATCHCLR | RTC_MODE0_CTRL_PRESCALER_DIV16;
  RTC->MODE0.COMP[0].reg = 1000 - 2;
  RTC->MODE0.READREQ.reg |= RTC_READREQ_RCONT;
  RTC->MODE0.INTENSET.reg = RTC_MODE0_INTENSET_CMP0;
  RTC->MODE0.CTRL.reg |= RTC_MODE0_CTRL_ENABLE;
  RTC->MODE0.INTFLAG.reg = 0xFF;
  NVIC_EnableIRQ(RTC_IRQn);
}

uint32_t RTC_get_count()
{
  RTC->MODE0.READREQ.reg |= RTC_READREQ_RREQ;
  return RTC->MODE0.COUNT.reg;
}


void RTC_Handler(void)
{
  millisecondcount++;
  RTC->MODE0.INTFLAG.reg = 0xFF;  // Clear all possible interrupts (all modes, too!)
}


uint32_t millis()
{
  return millisecondcount;
}


uint32_t micros()
{
  uint32_t thismicros;
  CpuCriticalVar();
  CpuEnterCritical(); 

#if DEBUG
  rtctick_microscalled++;
#endif
  thismicros = RTC->MODE0.COUNT.reg;
  if (RTC->MODE0.INTFLAG.bit.CMP0) {
    /*
     * Check for a ms interrupt pending but not yet
     * serviced, and correct for it.
     */
    thismicros = RTC->MODE0.COUNT.reg;  //re-read
    thismicros += 1000;
  }
  thismicros += 1000*millisecondcount;
	
#if DEBUG
  if (thismicros < rtctick_lastmicros) {
    // We shouldn't get here.  But allow it to BKPT
    rtctick_backwards++;
    rtctick_trace[rtctick_tracei].current = thismicros;
    rtctick_trace[rtctick_tracei].last = rtctick_lastmicros;
    rtctick_tracei = (rtctick_tracei+1) & 7;
  }
  rtctick_lastmicros = thismicros;
#endif
	
  CpuExitCritical();
  return thismicros;
}


void delay(uint32_t dtime)
{
  uint32_t starttm = micros();
  uint32_t endtime = dtime * 1000;
	
  while (micros() - starttm < endtime)
    ;
#if 0
  while (dtime > 0) {
    if ((micros() - start) >= 1000) {
      dtime--;
      start += 1000;
    }
  }
#endif
}

/*
 * delayMicroseconds
 * Delay for approximately n microseconds. Like the AVR version, does not
 * include interrupt time in the delay.
 */
 void delayMicroseconds(uint32_t delay)
 {

	 if (delay == 0)
		return;
#if 0
	 uint32_t starttm = micros();
	
  while (micros() - starttm < delay)
    ;
	return;
#else
   uint32_t lastcount, thiscount;

	 Rtc* myRTC = RTC;

   lastcount = myRTC->MODE0.COUNT.reg;
   while (delay--) {
     while (1) {
		 thiscount = myRTC->MODE0.COUNT.reg;
		 if (thiscount != lastcount)
			break;
      }// spin waiting for tick to change
     lastcount = thiscount;  // reload
   }
#endif
 }
