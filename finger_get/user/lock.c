#include "lock.h"
#include "usart.h"
#include "string.h"
#include "delay.h"

#define LOCK_MAX_SIZE 60

void usart2_tx_gpio_init(void){
	GPIO_InitTypeDef GPIO_InitStruct;
	 GPIO_InitStruct.Pin       = USART2_TX_PIN;
  GPIO_InitStruct.Mode      = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull      = GPIO_PULLUP;
	#if ((defined (USE_STM32F4XX_NUCLEO)) || (defined (USE_STM32L0XX_NUCLEO)))
  GPIO_InitStruct.Speed     = GPIO_SPEED_FAST;
	#endif
 
  HAL_GPIO_Init(USART2_TX_GPIO_PORT, &GPIO_InitStruct);
}

void send_cmd_to_lock(uint8_t cmd, uint8_t* data, uint8_t data_len){
	TMsg Msg;
	uint16_t temp_sum = 0;
	
	usart2_tx_gpio_init();
	HAL_GPIO_WritePin( USART2_TX_GPIO_PORT, USART2_TX_PIN, GPIO_PIN_RESET );
	Delay_nms(10);
	HAL_GPIO_WritePin( USART2_TX_GPIO_PORT, USART2_TX_PIN, GPIO_PIN_SET );
	Delay_nms(100);
	
	usart2_tx_AFGPIO_init();
	Msg.Data[0] = LOCK_HEADER;
	Msg.Data[3] = data_len + 1;
	Msg.Data[4] = cmd;
	temp_sum = Msg.Data[3];
	temp_sum += Msg.Data[4];
	for(uint8_t i = 0; i < data_len; i++){
		Msg.Data[i+5] = data[i];
		temp_sum += Msg.Data[i+5];
	}
	Msg.Data[1] = (uint8_t)(temp_sum >> 8);
	Msg.Data[2] = (uint8_t)temp_sum;
	Msg.Len = data_len + 5;
	
	UART_SendMsg(&Msg, USART2);
}

uint8_t process_lock_unit(uint8_t* data, uint8_t len){
	
	return 0;
}

void test_lock(void){
	uint8_t data[2] = {0};
	
	send_cmd_to_lock(LOCK_CMD_READ_REV, data, 0);
}
