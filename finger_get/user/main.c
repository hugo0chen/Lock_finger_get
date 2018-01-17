#include "stm32l0xx.h"
#include "stm32l0xx_hal.h"
#include "stm32l0xx_hal_conf.h"
#include "stm32l0xx_hal_def.h"
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
#define LED_RUNNING_PERIOD			1000
#define FINGER_RX_TIMEOUT     	30
#define SET_GET_FINGER_CMD    	0x01
#define UPLOAD_FINGER_INFO_CMD	0xA1

/****************************** Function Declaration ******************************/
volatile uint16_t node_ID = 0x0000;
UPDOWNLOADBUF up_download_finger;
extern Rx_para usart1_rx_flag;
extern UART_HandleTypeDef UartHandle1;

void finger_OpSta(void);
void process_fingerRxBuffer(void);
void process_serverRxBuffer(void);
void initDrivers(void);
void server_send(uint8_t* data, uint16_t data_len, uint8_t cmd);
void finger_module_upper_callBack (void);

void proces_lockUnitRxBuffer(void);

void SystemClock_Config(void){
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;
  
  __PWR_CLK_ENABLE();
  
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = 16;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLLMUL_4;
  RCC_OscInitStruct.PLL.PLLDIV = RCC_PLLDIV_2;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1);

  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/100); //10ms 

  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);
}

void initDrivers(void){
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

uint8_t flash_running_led(){
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

int main (){
	initDrivers();
	usart1_rx_flag.rx_status = 0;	
	finger.pkt_num = 0;
	finger.register_times = 0;
	finger.command = NO_CMD;
	
	//finger_fsm.fsm = REGISTER_UPPER;//test to del
	while(1){		
		//flash_running_led();
		if(lora_data.lora_data_arrived == 1){
			process_serverRxBuffer();
			lora_data.lora_data_arrived = 0;
		}

		if(finger_fsm.fsm != FSM_IDLE){
			if(finger_fsm.sta != STA_IDLE)
				finger_OpSta();
		}
					
		if((usart1_rx_flag.rx_status ==  1) && timeout(usart1_rx_flag.rx_timeTick, FINGER_RX_TIMEOUT)){
			usart1_rx_flag.rx_status = 0;
			process_fingerRxBuffer();			
		}
	}
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
	if(GPIO_Pin == GPIO_PIN_11){
		SX1278_Interupt();
	}
	if(GPIO_Pin == GPIO_PIN_0){
		if((finger_fsm.sta == STA_IDLE)&&(finger_fsm.fsm != FSM_IDLE)){
			if(finger.command == GENCHAR){
				finger_fsm.sta = GETIMG2_STA;
			}else{
				finger_fsm.sta = GETIMG1_STA;
			}
			finger_com.cmd_retry_times = 0;
			finger_com.response_ack_tickTime = local_ticktime();
		}
	}
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){	
//	if(huart->Instance == USART2){		
//		//todo
//	}
	if(huart->Instance == USART1){
		usart1_rx_flag.rx_status = 1;
		usart1_rx_flag.rx_timeTick = local_ticktime();
	}
}

//void HAL_UARTEx_WakeupCallback(UART_HandleTypeDef *huart){
//	//todo
//}

void process_fingerRxBuffer(void){
	uint16_t xBufferLen =  UartHandle1.RxXferSize - UartHandle1.RxXferCount;
	uint16_t startPostion = 0;
	uint16_t j = 0, m = 0;
	uint16_t frame_len = 0;
	
	/* Rx process is completed, restore huart->RxState to Ready */
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

	HAL_UART_Receive_IT(&UartHandle1, (uint8_t*)UART1_RxBuffer, UART1_RxBufferSize);
}
#define FINGER_MAX_CMD_RETYR_TIMES  5
#define FINGER_COM_ACK_TIME  100
#define FIGNER_COM_DATA_TIME 300
uint8_t finger_cmd_wait_timeOut(enum Finger_Register sta){
	if((sta == UPCHAR_STA) ||sta == DOWNCHAR_STA ){
		return timeout(finger_com.response_ack_tickTime, FIGNER_COM_DATA_TIME);
	}else{
		return timeout(finger_com.response_ack_tickTime, FINGER_COM_ACK_TIME);
	}
}
void shut_led(union SchdParameter led_no){	
	LED_OFF( LED_NO_0 );	
}

void call_failed_callback(uint8_t res){
	if(res == 0){
		LED_ON( LED_NO_0);
		Schd_After_Int(1000, shut_led, 0);
	}else if(res == 1){//第一次指纹采集
		LED_ON( LED_NO_1);
	}else if(res == 2){//第二次指纹采集
		LED_ON( LED_NO_2);
	}
	//todo TTS
}

void finger_OpSta(void){
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
				default:
					break;		
			}					
		}
		else{
			call_failed_callback(1);
			finger_com.cmd_retry_times = 0;
			finger_fsm.sta = STA_IDLE;
		}
		finger_com.response_ack_tickTime = local_ticktime();
	}
}

void process_serverRxBuffer(void){	
	uint8_t send_data[2] = {0};
	
	if((lora_data.lora_recv_data_buf[0] == 0xA5) && (lora_data.lora_recv_data_buf[1] == 0xA5)){
		if( (lora_data.lora_recv_data_buf[3] == (node_ID >> 8) ) && 
				(lora_data.lora_recv_data_buf[4] == (uint8_t)node_ID ) ){
			switch(lora_data.lora_recv_data_buf[5]){
				case SET_GET_FINGER_CMD:
					finger_fsm.fsm = REGISTER_UPPER;								
					LED_ON( LED_NO_1 );
					LED_ON( LED_NO_2 );
					Delay_nms(300);					
					send_data[0] = 0xFD;
					server_send(send_data, 1, SET_GET_FINGER_CMD);	
					LED_OFF( LED_NO_1 );
					LED_OFF( LED_NO_2 );
					break;
				case UPLOAD_FINGER_INFO_CMD: 
					if(lora_data.lora_recv_data_buf[6] == 0){
						finger_fsm.fsm = FSM_IDLE;
						LED_OFF( LED_NO_1 );
						LED_OFF( LED_NO_2 );
						up_download_finger.buf_size = 0;
					}else{
						finger_module_upper_callBack();
					}
					break;				
				default:
					break;
			}
		}
	}
	memset(lora_data.lora_recv_data_buf, 0, lora_data.lora_recv_data_buf_len);
	lora_data.lora_recv_data_buf_len = 0;
}
#define MAX_SEND_SIZE 210
void _buf_send_to_server(uint8_t*buf, uint8_t buf_len, uint8_t cmd, uint8_t pkt_no){
	uint8_t send_buf[MAX_SEND_SIZE] = {0x5A, 0x5A};
	uint16_t temp_sumCheck = 0;
	uint8_t i;
	
	temp_sumCheck = 0;
	send_buf[2] = buf_len + 7; 
	temp_sumCheck += send_buf[2];
	send_buf[3] = (uint8_t)(node_ID >> 8); 
	temp_sumCheck += send_buf[3];
	send_buf[4] = (uint8_t)node_ID; 
	temp_sumCheck += send_buf[4];
	send_buf[5] = cmd; 
	temp_sumCheck += send_buf[5];
	for( i = 0; i < buf_len; i++){
		send_buf[i + 6] = buf[i];
		temp_sumCheck += send_buf[i + 6];
	}
	send_buf[i + 6] = pkt_no;
	temp_sumCheck += send_buf[i + 6];
	send_buf[i + 7] = (uint8_t)(temp_sumCheck >> 8);
	send_buf[i + 8] = (uint8_t)(temp_sumCheck);
	Lora_Send(send_buf, send_buf[2] + 2);
}

#define PKT_MAX_SIZE 192
void server_send(uint8_t* data, uint16_t data_len, uint8_t cmd){
	uint8_t i,  pkt_num, pkt_remain, pkt_no;
	uint8_t* p = data;
	
	if(data_len > PKT_MAX_SIZE){
		pkt_num = data_len / PKT_MAX_SIZE;
		pkt_remain = data_len % PKT_MAX_SIZE;
		pkt_no = 0;
		for(i = 0; i < pkt_num; i++){
			pkt_no++;
			pkt_no = ( ( pkt_no == pkt_num ) ? 0: pkt_no);
			_buf_send_to_server(p, PKT_MAX_SIZE, cmd, pkt_no);	
			p += PKT_MAX_SIZE;
			Delay_nms(500); // to do tune the delayTime
		}
		if(pkt_remain != 0)		
			_buf_send_to_server(p, pkt_remain, cmd, 0);
	}else{
		_buf_send_to_server(data, data_len, cmd, 0);
	}	
}

void finger_module_upper_callBack (void) {
	
	server_send(up_download_finger.up_download_fingerModelBuff, up_download_finger.buf_size, UPLOAD_FINGER_INFO_CMD);
	LED_ON( LED_NO_1 );
	LED_ON( LED_NO_2 );
	//todo TTS
	finger.pkt_num = 0;
	up_download_finger.receive_data_flag = 0;
}




