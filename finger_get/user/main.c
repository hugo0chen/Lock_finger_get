#include "stm32l0xx.h"
#include "stm32l0xx_hal.h"
#include "stm32l0xx_hal_conf.h"
#include "stm32l0xx_hal_def.h"
#include "main.h"
#include "led.h"
#include "spi.h"
#include "lowpower.h"
#include "sx1276.h"
#include "usart.h"
#include "finger.h"
#include "lock.h"
#include "schd.h"
#include "globalvar.h"
#include "delay.h"
#include "string.h"


/****************************** Macro Definition ******************************/
const uint32_t node_ID = 0x0000;
UPDOWNLOADBUF up_download_finger;
uint32_t no_touch_timeOut_tick;

/*************************** Function Declaration ******************************/
void finger_OpSta(void);
void process_fingerRxBuffer(void);
void proces_lockUnitRxBuffer(void);

void process_serverRxBuffer(uint8_t serverType);
void server_send(uint8_t serverType, uint8_t* data, uint16_t data_len, uint8_t cmd);

static void SystemClock_Config(void){
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;
  
  __PWR_CLK_ENABLE();
  
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  
  RCC_OscInitStruct.OscillatorType 			= RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState 						= RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = 16;
  RCC_OscInitStruct.PLL.PLLState  			= RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource 			= RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL					= RCC_PLLMUL_4;
  RCC_OscInitStruct.PLL.PLLDIV 					= RCC_PLLDIV_2;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  RCC_ClkInitStruct.ClockType 		 = RCC_CLOCKTYPE_SYSCLK;
  RCC_ClkInitStruct.SYSCLKSource 	 = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1);

  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/100); 						//10ms 

  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);
}

static void initDrivers(void){
	__disable_irq();
  HAL_Init();
  SystemClock_Config();
	init_led();
	SPIx_Init();
	init_PowerControl_Configuration();
	USARTConfig();
	powerOn_LoRa();
	Lora_Init();
	init_finger();
	__enable_irq();
}

static uint8_t flash_running_led(){
	static uint8_t	ledStatus	= 0;
	static uint32_t runningTick	= 0;
   
	if ( timeout( runningTick, LED_RUNNING_PERIOD ) ){			
		if ( ledStatus == 0 ){
			LED_ON( LED_NO_0 );
			ledStatus = 1;
		} else {
			LED_OFF( LED_NO_0 );
			ledStatus = 0;
		}
		runningTick = local_ticktime();
		return 1;
	}
	return 0;
}

static void init_params(void){
	usart1_rx_flag.rx_status = 0;	
	usart2_rx_flag.rx_status = 0;
	finger.pkt_num = 0;
	finger.command = NO_CMD;
	up_download_finger.buf_size = 0;
	up_download_finger.finger_get_step = 0;
	up_download_finger.receive_data_flag = 0;
	memset(up_download_finger.finger_module_buf, 0, MAX_FINGER_BUFFERSIZE);
}

void update_no_touch_tickTime(void){
	no_touch_timeOut_tick = local_ticktime();
}

int main (){
	initDrivers();
	powerOn_finger();
	init_params();
	LED_ON( LED_NO_0 );
	LED_ON( LED_NO_1 );
	Delay_nms(1000);
	LED_OFF( LED_NO_0 );
	LED_OFF( LED_NO_1 );
	buzzer(1, 1000);
	update_no_touch_tickTime();
	while(1){	
		
		flash_running_led();
		//------ LORA -------
		if(lora_data.lora_data_arrived == 1){
			process_serverRxBuffer(DATA_FROM_SERVER_TYPE_LORA);
			lora_data.lora_data_arrived = 0;
		}
		//------ FINGER ------
		if(finger_fsm.fsm != FSM_IDLE){			
			finger_OpSta();
			if( timeout(no_touch_timeOut_tick, ONE_MINUTES)){
				buzzer( 1, 500);
				LED_OFF( LED_NO_1 );
				init_params();
				finger_fsm.fsm = FSM_IDLE;
				finger_fsm.sta = STA_IDLE;
				update_no_touch_tickTime();
			}
		}
					
		if((usart1_rx_flag.rx_status ==  1) && timeout(usart1_rx_flag.rx_timeTick, FINGER_UART_RX_TIMEOUT)){
			usart1_rx_flag.rx_status = 0;
			process_fingerRxBuffer();			
		}
		
		//----- USART2 external USB-UART_TTL ----
		if((usart2_rx_flag.rx_status ==  1) && timeout(usart2_rx_flag.rx_timeTick, LOCK_UART_RX_TIMEOUT)){
			usart2_rx_flag.rx_status = 0;
			process_serverRxBuffer(DATA_FROM_SERVER_TYPE_USB);		
		}
	}
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
	
	//----   LORA GDOx   ----
	if(GPIO_Pin == GPIO_PIN_11){
		SX1278_Interupt();
		HAL_NVIC_EnableIRQ((IRQn_Type)EXTI0_1_IRQn);
	}
	//----   finger touch   ----
	if(GPIO_Pin == GPIO_PIN_0){
		update_no_touch_tickTime();
		if(finger_fsm.fsm != FSM_IDLE){
			switch(finger_fsm.fsm){
				case REGISTER_UPPER_FSM:
					if(up_download_finger.finger_get_step == 1){
						finger_fsm.sta = GETIMG2_STA;
					}else{
						finger_fsm.sta = GETIMG1_STA;
					}
					break;
				case MATCH_LOCAL_FSM:				
					LED_OFF( LED_NO_1);
					finger_fsm.sta = GETIMG1_STA;
					break;
				default:
					break;
			}
			finger_com.cmd_retry_times = 0;
			finger_com.response_ack_tickTime = local_ticktime();
		}
	}
}

void set_usartX_flag(Rx_para *usart){
	(*usart).rx_status = 1;
	(*usart).rx_timeTick = local_ticktime();
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){	
	if(huart->Instance == USART2){		
		set_usartX_flag(&usart2_rx_flag);
	}
	if(huart->Instance == USART1){
		set_usartX_flag(&usart1_rx_flag);
	}
}

void process_fingerRxBuffer(void){
	uint16_t xBufferLen = (UartHandle1.RxXferSize - UartHandle1.RxXferCount);
	uint16_t startPostion = 0;
	uint16_t frame_len = 0;
	uint16_t j = 0, m = 0;
	
	UartHandle1.RxState = HAL_UART_STATE_READY;	
	do{
		startPostion = fingerSearchFrame(UART1_RxBuffer, xBufferLen);
		if(startPostion <  xBufferLen){
				finger.character_size = UART1_RxBuffer[7];
				finger.character_size = (finger.character_size << 8) + UART1_RxBuffer[8] - 2;
				m = (startPostion + 9);
				for( j = 0 ; j < finger.character_size; j++){
					finger.character_buf[j] = UART1_RxBuffer[m + j];
				}		
				switch(UART1_RxBuffer[startPostion + 6]){
					case PID_COMMAND:
							finger.pid = PID_COMMAND;
							finger_processCMD();
						break;
					case PID_DATA:
							finger.pid = PID_DATA;
							finger_processDATA();
						break;
					case PID_ACK:
							finger.pid = PID_ACK;
							finger_processACK();
						break;
					case  PID_END:
							finger.pid = PID_END;
							finger_processEND();
						break;
					default: break;
			 }
			frame_len = finger.character_size + 11;	
			if(xBufferLen > frame_len){
				for( j = 0; j < ( xBufferLen - frame_len ); j++){
					UART1_RxBuffer[j] = UART1_RxBuffer[frame_len + j];
				}
				for( j = xBufferLen - frame_len; j < xBufferLen; j++){
					UART1_RxBuffer[j] = 0;
				}
				xBufferLen = xBufferLen - frame_len;
				finger.character_size = 0;
			}else{
				finger.character_size = 0;
				break;
			}
		}else{
			break;
		}
	}while(1);

	memset( UART1_RxBuffer, 0, UART1_RxBufferSize);
	HAL_UART_Receive_IT(&UartHandle1, (uint8_t*)UART1_RxBuffer, UART1_RxBufferSize);
}

void shut_led(union SchdParameter led_no){	
	LED_OFF( LED_NO_0 );	
}

void finger_getStep(union SchdParameter step){
	up_download_finger.finger_get_step = 1;
}

void send_finger_data_to_server(void){
	server_send(DATA_FROM_SERVER_TYPE_LORA, up_download_finger.finger_module_buf, up_download_finger.buf_size, UPLOAD_FINGER_INFO_CMD);
	LED_ON( LED_NO_1 );
}

void finger_callback(uint8_t res){
	
	uint8_t send_buf[2] = {0x01, 0x0};
	
	switch(res){
		case 0:
			buzzer(6, 50);
			break;
		case 1:
			buzzer(1, 150);
			Schd_After_Int(200, finger_getStep, 0);
			break;
		case 2:
			buzzer(2, 150);			
			break;
		case 3:
			buzzer(3, 150);
			send_finger_data_to_server();
			break;
		case 4:
			buzzer(6, 50);			
			server_send(DATA_FROM_SERVER_TYPE_LORA, send_buf, 1, FAILED_ACK);
			break;
		default:
			break;
	}
}

#define TEMP_FLASH_ID 0x0001
void finger_OpSta(void){
	if(finger_fsm.sta != STA_IDLE){
		if(finger_cmd_wait_timeOut(finger_fsm.sta) ) {
			if(finger_com.cmd_retry_times < FINGER_MAX_CMD_RETYR_TIMES) {
				switch(finger_fsm.sta){
					case GETIMG1_STA:
					case GETIMG2_STA:
						sendCmd_getImg();
						break;
					case GENCHAR1_STA:
						sendCmd_genChar(FINGER_BUF_1_ID);
						break;
					case GENCHAR2_STA:
						sendCmd_genChar(FINGER_BUF_2_ID);
						break;
					case REGMODEL_STA:
						sendCmd_regModel();
						break;
					case UPCHAR_STA:
						sendCmd_upChar(FINGER_BUF_1_ID);
						break;
					case STORECHAR_STA:
						sendCmd_store(FINGER_BUF_1_ID, TEMP_FLASH_ID);
						break;
					case SEARCH_STA:
						sendCmd_search(FINGER_BUF_1_ID, FINGER_LIB_START_POSITION, SEARCH_PAGES_NUM);									
						break;
					default:
						break;		
				}					
			}
			else{
				finger_callback(0);
				finger_com.cmd_retry_times = 0;
				finger_fsm.sta = STA_IDLE;
			}
			finger_com.response_ack_tickTime = local_ticktime();
		}
	}
}

void process_serverRxBuffer(uint8_t serverType){	
	uint8_t send_data[2] = {0};
	uint8_t tempRxData[900] = {0};
	if(serverType == DATA_FROM_SERVER_TYPE_LORA){
		memcpy(tempRxData, lora_data.lora_recv_data_buf, lora_data.lora_recv_data_buf_len);
		memset(lora_data.lora_recv_data_buf, 0, lora_data.lora_recv_data_buf_len);
		lora_data.lora_recv_data_buf_len = 0;
	}else{
		memcpy(tempRxData, UART2_RxBuffer, (UartHandle2.RxXferSize - UartHandle2.RxXferCount));
		memset( UART2_RxBuffer, 0, UART2_RxBufferSize);
		HAL_UART_Receive_IT(&UartHandle2, (uint8_t*)UART2_RxBuffer, UART2_RxBufferSize);
	}
	
	if((tempRxData[0] == FROM_SERVER_HEADER_1) && (tempRxData[1] == FROM_SERVER_HEADER_2)){
		if( (tempRxData[3] ==( uint8_t)(node_ID >> 24) ) && (tempRxData[4] == (uint8_t)(node_ID >> 16) )&&
				(tempRxData[5] == (uint8_t)(node_ID >> 8) )&&(tempRxData[6] ==( uint8_t)node_ID ) ){
			switch(tempRxData[7]){
				case SET_GET_FINGER_CMD:
					finger_fsm.fsm = REGISTER_UPPER_FSM;								
					LED_ON( LED_NO_1 );	
					send_data[0] = 0x00;
					server_send(DATA_FROM_SERVER_TYPE_LORA, send_data, 1, SET_GET_FINGER_CMD);					
					buzzer(1, 300);
					LED_OFF( LED_NO_1 );
					init_params();
					update_no_touch_tickTime();				
					break;
				case UPLOAD_FINGER_INFO_CMD:
					if(tempRxData[8] == 0){
						init_params();
						finger_fsm.fsm = FSM_IDLE;	
						buzzer(4, 150);
						LED_OFF( LED_NO_1 );						
					}
					break;				
				default:
					break;
			}
		}
	}
}
#define MAX_SEND_SIZE 210
void _buf_send_to_server(uint8_t*buf, uint8_t buf_len, uint8_t cmd, uint8_t pkt_no){
	uint8_t send_buf[MAX_SEND_SIZE] = {TO_SERVER_HEADER_1, TO_SERVER_HEADER_2};
	uint16_t temp_sumCheck = 0;
	uint8_t i;
	
	temp_sumCheck = 0;
	send_buf[2] = buf_len + 9; 
	temp_sumCheck += send_buf[2];
	send_buf[3] = (uint8_t)(node_ID >> 24); 
	temp_sumCheck += send_buf[3];
	send_buf[4] = (uint8_t)(node_ID >> 16); 
	temp_sumCheck += send_buf[4];
	send_buf[5] = (uint8_t)(node_ID >> 8); 
	temp_sumCheck += send_buf[5];
	send_buf[6] = (uint8_t)node_ID; 
	temp_sumCheck += send_buf[6];
	send_buf[7] = cmd; 
	temp_sumCheck += send_buf[7];
	for( i = 0; i < buf_len; i++){
		send_buf[i + 8] = buf[i];
		temp_sumCheck += send_buf[i + 8];
	}
	send_buf[i + 8] = pkt_no;
	temp_sumCheck += send_buf[i + 6];
	send_buf[i + 9] = (uint8_t)(temp_sumCheck >> 8);
	send_buf[i + 10] = (uint8_t)(temp_sumCheck);
	Lora_Send(send_buf, send_buf[2] + 2);
}

void uart_server(uint8_t* data, uint16_t data_len){
	TMsg msg;
	memcpy(msg.Data, data, data_len);
	UART_SendMsg(&msg, USART2);
}

#define PKT_MAX_SIZE 192
void server_send(uint8_t serverType, uint8_t* data, uint16_t data_len, uint8_t cmd){
	uint8_t i,  pkt_num, pkt_remain, pkt_no;
	uint8_t* p = data;
	
	if(serverType == DATA_FROM_SERVER_TYPE_USB){
		uart_server(data, data_len);
	}else if(serverType == DATA_FROM_SERVER_TYPE_LORA){
		LORA_RESPONSE:
		if(data_len > PKT_MAX_SIZE){
			pkt_num = data_len / PKT_MAX_SIZE;
			pkt_remain = data_len % PKT_MAX_SIZE;
			pkt_no = 0;
			for(i = 0; i < pkt_num; i++){
				pkt_no++;
				pkt_no = ( ( pkt_no == pkt_num ) ? 0: pkt_no);
				_buf_send_to_server(p, PKT_MAX_SIZE, cmd, pkt_no);	
				p += PKT_MAX_SIZE;
			}
			if(pkt_remain != 0)		
				_buf_send_to_server(p, pkt_remain, cmd, 0);
		}else{
			_buf_send_to_server(data, data_len, cmd, 0);
		}	
	}else{
		uart_server(data, data_len);
		goto LORA_RESPONSE;
	}
}
//---  end ---
