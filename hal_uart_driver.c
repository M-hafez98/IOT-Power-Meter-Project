#include "hal_uart_driver.h"
#include "msp.h"
#include "string.h"

uint8_t rx_data_buffer[300];
volatile uint8_t data_len = 0;
uint8_t flag = 0x00;
extern uint8_t* bearer;

static inline void clear_regs(UART0_MemMapPtr u){
  u->BDH &= 0x00;
  u->BDL &= 0x00;
  u->C1 &= 0x00;
  u->C2 &= 0x00;
  u->C3 &= 0x00;
  u->C4 &= 0x00;
  u->C5 &= 0x00;
  u->MA1 &= 0x00;
  u->MA2 &= 0x00;
  u->S1 &= 0x00;
  u->S2 &= 0x00;
}

static inline void set_data_length(UART0_MemMapPtr u, uint8_t d){
	if(d == FRAME_FORMAT_9_BIT){
		u->C1 |= UART0_C1_M_MASK;
	}
	else{
		u->C1 &= ~UART0_C1_M_MASK;
	}
}
static inline void set_stop_bits(UART0_MemMapPtr u, uint8_t s){
	if(s == TWO_STOP_BIT){
		u->BDH |= UART0_BDH_SBNS_MASK;
	}
	else{
		u->BDH &= ~UART0_BDH_SBNS_MASK;
	}
}
static inline void set_oversampling(UART0_MemMapPtr u, uint8_t s){
  u->C4 = s;
}
static inline void set_baud_rate(UART0_MemMapPtr u, uint32_t b){
  /* MCGFLL_Clock = 20971520U which is selected */
  uint32_t MCGFLL_Clock = 20971520U;
  uint16_t SBR;
  SBR = (uint16_t)(MCGFLL_Clock/((16)*b)); 
  //u->BDH = 0x00;
  u->BDH = 0x1F & ((uint8_t)(SBR>>8));        //three MSBs = 0
  u->BDL = (uint8_t)(SBR & 0x00FF);
}

static inline void parity(UART0_MemMapPtr u, uint8_t p){
	if(p == PARITY){
		u->C1 |= UART0_C1_PE_MASK;
	}
	else{
		u->C1 &= ~UART0_C1_PE_MASK;
	}
}
static inline void parity_type(UART0_MemMapPtr u, uint8_t pt){
	if(pt == ODD_PARITY){
		u->C1 |= UART0_C1_PT_MASK;
	}
	else{
		u->C1 &= ~UART0_C1_PT_MASK;
	}
}
static inline void config_errors_interrupt(UART0_MemMapPtr u){
	u->C3 |= UART0_C3_PEIE_MASK | UART0_C3_FEIE_MASK | UART0_C3_NEIE_MASK | UART0_C3_ORIE_MASK;
}

void enable_uart(UART0_MemMapPtr u){
	u->C1 &= ~UART0_C1_LOOPS_MASK; // normal operation
	u->C2 |= UART0_C2_RE_MASK | UART0_C2_TE_MASK;
}
void disable_uart(UART0_MemMapPtr u){
	u->C2 &= ~UART0_C2_RE_MASK;
	u->C2 &= ~UART0_C2_TE_MASK;
}
void uart_init(uart_handel_t *h){
        clear_regs(h->instance);
	set_data_length(h->instance, h->config.data_length);
	set_stop_bits(h->instance, h->config.stop_bits);
        //set_oversampling(h->instance, h->config.over_sampling);
	set_baud_rate(h->instance, h->config.baud_rate);
	parity(h->instance, h->config.parity_e);
	parity_type(h->instance, h->config.parity_t);
        enable_uart(h->instance); // to enable uart tx and rx
}

/*
	@param: h is uart_config structure handling
			data_buffer is a pointer to the data array to be sent
			len is the length of the array
*/
void uart_tx(uart_handel_t* h, uint8_t* data_buffer, uint8_t len){
	h->pTxBuffer = data_buffer;
	h->TxCount = len;
	h->instance->C2 |= UART0_C2_TIE_MASK; // enable interrupt for TDRE
}
void uart_rx(uart_handel_t* h){
	h->pRxBuffer = rx_data_buffer;
	h->RxCount = 0;
	h->instance->S1 |= UART0_S1_FE_MASK; // to clear FE flag since it is set initially
        config_errors_interrupt(h->instance);
        (void)h->instance->D; // to avoid the zero/residual value of D to make interrupt
	h->instance->C2 |= UART0_C2_RIE_MASK; // enable interrupt for full buffer detection
}
static void tx_isr(uart_handel_t* h){
	h->instance->D = *h->pTxBuffer; // only takes the fitst byte in the array of pTxBuffer to send
	(h->TxCount)--; // one data byte has been sent
        
	if(h->TxCount == 0){
          h->instance->C2 &= ~UART0_C2_TIE_MASK; // disable TDRE interrupt
	}
        else{
          h->pTxBuffer++;
        }
}

static void rx_isr(uart_handel_t* h){
    *h->pRxBuffer = h->instance->D; // this should clear RDRF flag
    (h->RxCount)++;
    data_len = h->RxCount;
    
    if( ( ( ((*h->pRxBuffer) == '\n') && ( (*(h->pRxBuffer-1)) == '\r') ) && ( ((*(h->pRxBuffer-2)) == 'K') || ((*(h->pRxBuffer-2)) == 'R') || ((*(h->pRxBuffer-2)) == 'D') ) ) || ( ((*h->pRxBuffer) == '\n') && ((*(h->pRxBuffer-1)) == '\r') && (flag == 0x03) && (((*(h->pRxBuffer+1)) != '+')) ) ){
    h->instance->C2 &= ~UART0_C2_RIE_MASK; // disable full D buffer interrupt
    /*disable all error interrupts*/
    h->instance->C3 &= ~UART0_C3_PEIE_MASK & ~UART0_C3_FEIE_MASK & ~UART0_C3_NEIE_MASK & ~UART0_C3_ORIE_MASK;
    }
    else{
      h->pRxBuffer++;
    }
}

static inline void clear_error_interrupt(uart_handel_t* h){
	volatile uint8_t val;
	val = h->instance->S1;
	val = h->instance->D;
}

void uart_isr_handler(uart_handel_t* h){
	uint8_t tem1, tem2;
        volatile uint8_t val;
	/* check for errors interrupts */
	tem1 = h->instance->S1 & UART0_S1_FE_MASK;
	tem2 = h->instance->C3 & UART0_C3_FEIE_SHIFT;
	if(tem1 && tem2){
		h->uart_error = UART_FRAME_ERROR;
	}
	
	tem1 = h->instance->S1 & UART0_S1_OR_MASK;
	tem2 = h->instance->C3 & UART0_C3_ORIE_MASK;
	if(tem1 && tem2){
		h->uart_error = UART_OVER_RUN_ERROR;
	}
	
	tem1 = h->instance->S1 & UART0_S1_NF_MASK;
	tem2 = h->instance->C3 & UART0_C3_NEIE_MASK;
	if(tem1 && tem2){
		h->uart_error = UART_NOISE_ERROR;
	}
	
	tem1 = h->instance->S1 & UART0_S1_PF_MASK;
	tem2 = h->instance->C3 & UART0_C3_PEIE_MASK;
	if(tem1 && tem2){
		h->uart_error = UART_PARITY_ERROR;
	}
	
	if(h->uart_error == UART_PARITY_ERROR){
		// do certain action
          flag = 1<<0;
          while(1);
	}
	else if(h->uart_error != UART_NO_ERROR){
          clear_error_interrupt(h);
          flag = 1<<1;
          while(1);
	}
	else{
		/* tx interrupt */
		tem1 = h->instance->S1 & UART0_S1_TDRE_MASK;
		tem2 = h->instance->C2 & UART0_C2_TIE_MASK;
		if(tem1 && tem2){
                  val = h->instance->S1; // this is to clear tx flag in addition to writing in data register
                  tx_isr(h);
		}
                /* rx interrupt */
                tem1 = h->instance->S1 & UART0_S1_RDRF_MASK;
                tem2 = h->instance->C2 & UART0_C2_RIE_MASK;
                if(tem1 && tem2){
                  val = h->instance->S1;
                  rx_isr(h);
                }
	}
}