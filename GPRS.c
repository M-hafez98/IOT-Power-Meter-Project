#include "gprs.h"
#include "hal_uart_driver.h"

/*global variables*/
extern uart_handel_t huart;
extern uint32_t g_seconds;
extern uint8_t flag;
modem_status_t comm_state = POWER_OFF;
uint8_t* baud_rate_ident = "AT\r";
uint8_t* no_echo = "ATE0&W\r";
uint8_t* serverNumber;
float meter_info[30]={220,220,220,20,20,20,0,1800,100,1400,1500};
uint8_t meter_info_str[200];
uint8_t meter_electrical_info_str[200]= "meter_id=1&Vr=";
extern uint8_t rx_data_buffer[300];
extern uint8_t data_len;
uint8_t http_read_response[300];
uint8_t http_action_response[200];
uint8_t* bearer_commands[5] = {"AT+SAPBR=3,1,\"Contype\", \"GPRS\"\r",
                               "AT+SAPBR=3,1,\"APN\",\"internet.vodafone.net\"\r",
                               "AT+SAPBR=1,1\r",
                               "AT+SAPBR=2,1\r",
                               "AT+SAPBR=0,1\r"};

uint8_t* http_get_commands[6] = {"AT+HTTPINIT\r",
                                 "AT+HTTPPARA=\"CID\",1\r",
                                 "AT+HTTPPARA=\"URL\",\"http://edge2018-001-site19.gtempurl.com/api/test_meter/insert_meter?unit_id=60&msg=HiIamSIM281724\"\r",
                                 "AT+HTTPACTION=0\r",
                                 "AT+HTTPREAD\r",
                                 "AT+HTTPTERM\r"};

uint8_t* http_post_commands[7] = {"AT+HTTPINIT\r",
                                  "AT+HTTPPARA=\"CID\",1\r",
                                  "AT+HTTPPARA=\"URL\",\"http://edge2018-001-site19.gtempurl.com/api/test_meter/post_meter2\"\r",
                                  "AT+HTTPPARA=\"CONTENT\",\"application/x-www-form-urlencoded\"\r",
                                  "AT+HTTPDATA=100,20000\r",
                                  "AT+HTTPACTION=1\r",
                                  "AT+HTTPREAD\r"};

uint8_t* https_post_commands[8] = {"AT+HTTPINIT\r",
                                   "AT+HTTPPARA=\"CID\",1\r",
                                   "AT+HTTPPARA=\"URL\",\"https://edge2018-001-site19.gtempurl.com/api/test_meter/post_meter2\"\r",
                                   "AT+HTTPSSL=1\r",/*test the position of this cmd*/
                                   "AT+HTTPPARA=\"CONTENT\",\"application/x-www-form-urlencoded\"\r",
                                   "AT+HTTPDATA=20,20000\r",
                                   "AT+HTTPACTION=1\r",
                                   "AT+HTTPREAD\r"};

http_message_t mes = {"unit_id=",66,"&msg=bpost\r"};
uint8_t http_url[240] = "AT+HTTPPARA=\"URL\",\"http://edge2018-001-site19.gtempurl.com/api/test_meter/add_meter_readings\"\r";

void GPRS_Power(uint8_t ON_OFF){
  uint32_t temp;
  static uint32_t warm_up_timer = 0;
  /* power on */
  if((ON_OFF == 1) && (comm_state == POWER_OFF)){
    PTE->PSOR |=(1U<<GPRS_PWR); //GPRS power converter ON
    PTE->PSOR |=(1U<<GPRS_KEY); //GPRS power key ON
    temp = g_seconds;
    comm_state = POWERING_UP;
    while(g_seconds < (temp+2));
    PTE->PCOR |=(1U<<GPRS_KEY); //release GPRS power key
    /*wait 5 seconds or more for modem to startup*/
    if(warm_up_timer==0){
      warm_up_timer = g_seconds;
    }
    if(g_seconds < (warm_up_timer+5)){
      comm_state = WARMING_UP;
    }
    comm_state = POWER_ON; 
  }
  /* power off */
  else{
    PTE->PCOR |=(1U<<GPRS_PWR); //shut power converter off
    PTE->PCOR |=(1U<<GPRS_KEY); //release power key (double check)
    comm_state = POWER_OFF;
  }
}

/*this function is called by rtc irq handler due to the required delay that gprs is needed*/
void set_modem_buad_rate(void){
  uart_tx(&huart,baud_rate_ident,3);
  //UART0_puts("AT\r");  
}

/* called by the application */
uint32_t modem_init(void){
  /* power up the modem */
  GPRS_Power(1);
  /* enable rx interrupt */
  uart_rx(&huart);
  /* set the baudrate and check for the reciption string */
  //set_modem_buad_rate(); doesn't work, as it requires some delay. 
  
  while((huart.instance->C2 & UART0_C2_RIE_MASK) == UART0_C2_RIE_MASK);
  comm_state = parse_buffer();
  
  return comm_state;
}

modem_status_t parse_buffer(void){
  if((rx_data_buffer[0] == 'A') && (rx_data_buffer[6] == 'K')){
    comm_state = MODEM_READY;
    //disable_echo();
  }
  else{
    comm_state = POWER_ON;
  }
  return comm_state;
}

uint8_t* gprs_config(uint8_t* b){
  uint8_t last;
  uint8_t preLast;
  uint8_t eLetter;
  uint8_t tx_rx_number = 0;
  do{
    tx_rx_number++;
    uart_rx(&huart);//enable interrupt for the next reciption response to the following tx 
    uart_tx(&huart,b,strlen((char*)b));
/*wait in order to avoid another code to call uart_rx or _tx while they are actually running*/
    while(((huart.instance->C2 & UART0_C2_TIE_MASK) == UART0_C2_TIE_MASK) || ((huart.instance->C2 & UART0_C2_RIE_MASK) == UART0_C2_RIE_MASK));
    last = rx_data_buffer[data_len-3];
    preLast = rx_data_buffer[data_len-4];
    eLetter = rx_data_buffer[data_len-7];/* E for Error letter */
    if(tx_rx_number >= 3){ // repeat txing and rxing twice in case of error occurs
      break;
    }
  }while( ((preLast != 'O') && (last != 'K') && (last != 'D') ) || ((eLetter == 'E') && (last == 'R')) );
  if( (preLast == 'O') && (last == 'K') ){
    return "OK";
  }
  else if( last == 'D' ){
    return "Download";
  }
  else{
    return NULL;
  }
  /** do here smth checks that the previous stage is done correctly **/
}

uint8_t* ip_config(void){
  uint8_t iCommand;
  uint8_t* status = "OK";
  for(iCommand=0; iCommand<4; iCommand++){
    if(status == NULL){
      break;
    }
    else{
      status = gprs_config(bearer_commands[iCommand]);
      clear_buff(rx_data_buffer);
    }
  }
  if(status == NULL){
    return "Failed";
  }
  else{
    return "Done";
  }
}

uint8_t* http_get_config(void){
  uint8_t iCommand;
  uint8_t* status = "OK";
  for(iCommand=0; iCommand<5; iCommand++){
    if(status == NULL){
      break;
    }
    else{
      status = gprs_config(http_get_commands[iCommand]);
      if((*status == 'O') && (iCommand == 3)){
        flag = 0x03;
        uart_rx(&huart); /* enable the rx interrupt for the rest of http action response */
        while((huart.instance->C2 & UART0_C2_RIE_MASK) == UART0_C2_RIE_MASK);
        handel_http_response(http_action_response);
        flag = 0x00;
        //clear_buff(rx_data_buffer);
        serverNumber = (uint8_t*)strchr((char*)http_action_response,'2');
        if(*serverNumber != '2'){
          iCommand = 2;
        }
      }
      if(iCommand == 4){
        handel_http_response(http_read_response);
      }
      clear_buff(rx_data_buffer);
    }
  }
  
  terminate_http();
  close_context();
  
  if(status == NULL){
    return "Failed";
  }
  else{
    return "Done";
  }
}

uint8_t* http_post_config(void){
  uint8_t iCommand;
  //uint8_t* b = "Vr=220&Vy=220&Vb=220&Ir=20&Iy=20&Ib=20&In=0&Pa=1800&Pb=100&Pc=1400&Ea=1200\r";
  //uint16_t bSize = strlen((char*)b);
  obtain_meter_info(meter_info);
  convert_to_str(meter_info);
  fill_meter_info_str(meter_info_str);
  //uint8_t* b="unit_id=99&msg=post\r";/*this will be the array of meter_electrical_info_str*/
  uint8_t* status = "OK";
  for(iCommand=0; iCommand<7; iCommand++){
    if(status == NULL){
      break;
    }
    else{
      if((*status == 'O') && (iCommand == 2)){ /* url command */
        status = gprs_config(http_url);
      }
      else{
        status = gprs_config(http_post_commands[iCommand]);
        if((*status == 'D') && (iCommand == 4)){
          clear_buff(rx_data_buffer);
          uart_rx(&huart); /* enable the rx interrupt for the rest of http action response */
          //uart_tx(&huart,mes.uID,strlen((char*)mes.uID));
          //uart_tx(&huart,&mes.id,1);
          //uart_tx(&huart,mes.message,strlen((char*)mes.message));
          uart_tx(&huart,meter_electrical_info_str,strlen((char*)meter_electrical_info_str));
          while((huart.instance->C2 & UART0_C2_RIE_MASK) == UART0_C2_RIE_MASK);
          if(rx_data_buffer[2] == 'O'){
            status = "OK";
          }
        }
        if((*status == 'O') && (iCommand == 5)){
          flag = 0x03;
          uart_rx(&huart); /* enable the rx interrupt for the rest of http action response */
          while((huart.instance->C2 & UART0_C2_RIE_MASK) == UART0_C2_RIE_MASK);
          handel_http_response(http_action_response);
          flag = 0x00;
          //clear_buff(rx_data_buffer);
          serverNumber = (uint8_t*)strchr((char*)http_action_response,'2');
          if(*serverNumber != '2'){
            iCommand = 2;
          }
        }
        if(iCommand == 6){
          handel_http_response(http_read_response);
        }
     }
     clear_buff(rx_data_buffer);
    }
  }
  
  terminate_http();
  close_context();
  
  if(status == NULL){
    return "Failed";
  }
  else{
    return "Done";
  }
}

void handel_http_response(uint8_t* r){
  uint8_t i;
  for(i=0; i<data_len; i++){
    r[i] = rx_data_buffer[i];
  }
}

void close_context(void){
  (void)gprs_config(bearer_commands[4]);
}

void terminate_http(void){
  (void)gprs_config(http_get_commands[5]);
  clear_buff(rx_data_buffer);
}

void clear_buff(uint8_t* b){
  static uint16_t i;
  for(i=0; i<data_len; i++){
    b[i] = 0;
  }
  data_len = 0;
}

void disable_echo(void){
  clear_buff(rx_data_buffer);
  uart_rx(&huart);
  uart_tx(&huart,no_echo,6);
  while((huart.instance->C2 & UART0_C2_RIE_MASK) == UART0_C2_RIE_MASK);
}

void fill_url(void){
  //uint8_t i;
  uint8_t iSize = strlen((char*)http_url);
  for(;iSize < 235;iSize++){
    *(http_url+iSize) = '8';
  }
  
  *(http_url+iSize++) = '\"';
  *(http_url+iSize++) = '\r';
  *(http_url+iSize++) = '\n';
  *(http_url+iSize++) = '\0';
}
/* info is passed by the meter_info variable */
void obtain_meter_info(float* info){
  uint8_t i;
  Meter_Info_t value = Phase_R_Voltage;
  for(i=0;i<METER_INFO_VALUES;i++){
    info[i] = Meter_Get_Info(value);
    value++;
  }
}
/* info is passed by the meter_info variable */
void convert_to_str(float* info){
  uint8_t i,j;
  uint8_t size=0;
  for(i=0;i<METER_INFO_VALUES;i++){
    //fcvt(info[i],6,(char*)meter_info_str);
    sprintf((char*)meter_info_str+size,"%f",info[i]);
    size = strlen((char*)meter_info_str);
    /*leave two decimal points only*/
    for(j=0;j<4;j++){
      *(meter_info_str+size-1) = 0;
      size--;
    }
    *(meter_info_str+size) = ',';
    size++;
  }
}
/* info_str is passed by the meter_info_str variable */
void fill_meter_info_str(uint8_t* info_str){
  uint8_t* eQuantity = "&Vy=&Vb=&Ia=&Iy=&Ib=&In=&Pa=&Pb=&Pc=&Et=";
  uint16_t i, j, k=0;
  uint16_t eSize = strlen((char*)eQuantity);
  uint16_t mSize = strlen((char*)meter_electrical_info_str);
  /* loop for adding the number */
  for(i=0;i<strlen((char*)info_str);i++){
    while(*(info_str+i) != ','){
      *(meter_electrical_info_str+mSize) = *(info_str+i);
      i++;
      mSize++;
    }
    /* loop for adding 4 chars from the eQuantity array */
    while(k<eSize){
      for(j=k;j<(k+4);j++){
      *(meter_electrical_info_str+mSize+j-k) = *(eQuantity+j);
      }
      k=j;
      mSize += 4;
      break;
    }
  }
}