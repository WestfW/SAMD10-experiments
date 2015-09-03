#include <stdint.h>

/*
 * wiring_digital.c
 */

#define INPUT 0
#define OUTPUT 1

extern const int8_t digital_pin_to_pinno[];
extern void __attribute__ ((noinline)) pinMode(uint8_t, uint8_t);
extern void __attribute__ ((noinline)) digitalWrite(uint8_t, uint8_t);


/*
 * ticker.c
 */
extern uint32_t __attribute__ ((noinline)) millis(void);
extern uint32_t __attribute__ ((noinline)) micros(void);
extern uint32_t __attribute__ ((noinline)) RTC_get_count(void);
extern void __attribute__ ((noinline)) delay(uint32_t);
extern void __attribute__ ((noinline)) RTC_init(void);
extern void delayMicroseconds(uint32_t delay);
 
/*
 * hardwareserial.c
 */
void uart_init_(int baud_d);
// Try to get the baud rate divisor calculated at compile time
static inline void uart_init(uint32_t baud) {
	int br = 65536ULL * (F_CPU - 16ULL * baud) / F_CPU;
	uart_init_(br);
}
extern void uart_putc(char c);
extern int uart_getc(void);
extern void uart_puts(const char *s);

