#include "hal_uart_driver.h"
#include "msp.h"

/* preprocessor directives */
#define CPU_MKL34Z64VLH4


void MX_uart_init(uart_handel_t*);
void callbackFunction(void);
void timer_delay_s(uint16_t);