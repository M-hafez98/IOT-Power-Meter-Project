#ifndef GPRS_H
#define GPRS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>				
#include "peripherals.h"
#include "battery.h"
//#include "uart.h"
#include "application.h"
#include "Meter_Info.h"

#define HTTP_POST
#define HTTPS_POST
#define METER_INFO_VALUES 11U

typedef enum{
  POWER_OFF = 0x00,
  POWER_ON,
  WARMING_UP,
  MODEM_READY,
  POWERING_UP
}modem_status_t;

typedef struct{
  uint8_t* uID; /* length = 8*/
  uint8_t id;
  uint8_t* message; /* length = 11 */
}http_message_t;

/*******************************************************************************/
void GPRS_Power(uint8_t ON_OFF);
void set_modem_buad_rate(void);
uint32_t modem_init(void); // called from the application
modem_status_t parse_buffer(void);
void disable_echo(void);
uint8_t* gprs_config(uint8_t*);
uint8_t* ip_config(void);
void close_context(void);
uint8_t* http_get_config(void);
uint8_t* http_post_config(void);
void terminate_http(void);
void clear_buff(uint8_t*);
void fill_url(void);
void handel_http_response(uint8_t*);
void obtain_meter_info(float*);
void convert_to_str(float*);
void fill_meter_info_str(uint8_t*);
/*******************************************************************************/


#endif