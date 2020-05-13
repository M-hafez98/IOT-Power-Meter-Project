#include "app.h"
#include "clock_config.h"
#include "application.h"
#include "configs.h"
#include "calibration.h"
#include "assert.h"
#include "delay.h"
#include "string.h"
#include "GPRS.h"

/* global variables */
uart_handel_t huart = {0}; // handel struct for uart0 peripheral
extern uint8_t rx_data_buffer[300];
extern uint8_t data_len;
extern uint8_t flag;
uint32_t modem_status;
uint8_t* bearer_config_status;
uint8_t* http_config_status;

void main(void)
{
  Hardware_Initialization();
  Calibration();
  /* enable uart0 clk, pin mux config and enable prot clk and NVIC enable for uart0 */
  UART_MspInit();
  /* uart0 parameter configuration */
  MX_uart_init(&huart);
  /* timer0 initialization */
  TPM_MspInit();
  
  modem_status = modem_init();
  
  while(1){
    if(modem_status == MODEM_READY){
      bearer_config_status = ip_config();
      if(*bearer_config_status == 'D'){
      #ifdef HTTP_POST
        http_config_status = http_post_config();
      #else
        http_config_status = http_get_config();
      #endif
      }
    }
  
  timer_delay_s(300); /* 2 min delay */
  
  //Application_Initialization();
  //Application_Start();
  }
  
}

void callbackFunction(void){
  uart_isr_handler(&huart);
}

void MX_uart_init (uart_handel_t* h){
  h->instance = UART0;
  h->config.data_length = FRAME_FORMAT_8_BIT;
  h->config.baud_rate = 9600U;
  h->config.stop_bits = ONE_STOP_BIT;
  h->config.parity_e = NO_PARITY;
  h->config.parity_t = 0;
  h->config.over_sampling = OVER_SAMPLE; // for 16 oversampling 0x0F
  uart_init(h);
}

void timer_delay_s(uint16_t loop){
  uint16_t i;
  TPM0->SC |= (TPM_SC_CMOD_MASK & 0x08); // turn on timer
  for(i=0;i<loop;i++){
    while( !( (TPM0->SC & TPM_SC_TOF_MASK) == TPM_SC_TOF_MASK ) ); // gives 0.4 s
    TPM0->SC |= TPM_SC_TOF_MASK; // clear overflow falg
  }
}