#include "MKL34Z4.h"
#include "stdint.h"
#include "stdlib.h"

/* Configuration Constans */
#define BAUD_RATE_9600 1
#define BAUD_RATE_115200 2
#define FRAME_FORMAT_9_BIT 1
#define FRAME_FORMAT_8_BIT 0
#define TWO_STOP_BIT 1
#define ONE_STOP_BIT 0
#define PARITY 1
#define NO_PARITY 0
#define ODD_PARITY 1
#define EVEN_PARITY 0
#define OVER_SAMPLE 0x0F

/* Interrupt Enable Bits */

/* UART structs and enums for data and status handling */
typedef enum{
	UART_TX_READY,
	UART_TX_BUSY,
	UART_RX_READY,
	UART_RX_BUSY,
	UART_ERROR
}uart_status_t;

typedef enum{
	UART_NO_ERROR = 0x00,
	UART_FRAME_ERROR,
	UART_PARITY_ERROR,
	UART_OVER_RUN_ERROR,
	UART_NOISE_ERROR
}uart_error_t;

typedef struct{
  uint8_t data_length;
  uint8_t stop_bits;
  uint32_t baud_rate;
  uint8_t parity_e;
  uint8_t parity_t;
  uint8_t over_sampling;
}uart_config_t;

typedef struct{
	UART0_MemMapPtr instance; // UART Module base address
	uart_config_t config; // UART config bits
	uint8_t* pTxBuffer;
	uint8_t TxCount; // the count of data bytes to be send
	uint8_t* pRxBuffer;
	uint16_t RxCount;
	uart_status_t rx_status;
	uart_status_t tx_status;
	uart_error_t uart_error;
}uart_handel_t;


/* UART intialization function and associated helper functions */
void uart_init(uart_handel_t*);
void UART_MspInit(void);
static inline void clear_regs(UART0_MemMapPtr);
static inline void set_data_length(UART0_MemMapPtr, uint8_t);
static inline void set_stop_bits(UART0_MemMapPtr, uint8_t);
static inline void set_oversampling(UART0_MemMapPtr, uint8_t);
static inline void set_baud_rate(UART0_MemMapPtr, uint32_t);
static inline void parity(UART0_MemMapPtr, uint8_t);
static inline void parity_type(UART0_MemMapPtr, uint8_t);
static inline void config_errors_interrupt(UART0_MemMapPtr);
 
/* enable and disable the UART */ 
void enable_uart(UART0_MemMapPtr);
void disable_uart(UART0_MemMapPtr);

/* UART Transmission */
void uart_tx(uart_handel_t*, uint8_t*, uint8_t);

/* UART Reciption */
void uart_rx(uart_handel_t*);

/* Interrupt handellers */
static void tx_isr(uart_handel_t*);
//static void handel_tx_complete_interrupt(uart_handel_t*);
static void rx_isr(uart_handel_t*);
void uart_isr_handler(uart_handel_t*);

/* clear error interrupt function */
static inline void clear_error_interrupt(uart_handel_t*);
