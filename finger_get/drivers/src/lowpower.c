#include "lowpower.h"
#include "stm32l0xx_hal.h"

void init_PowerControl_Configuration(void){
	GPIO_InitTypeDef	GPIO_InitStructure;

	GPIO_InitStructure.Pin 	 = GPIO_PIN_1;
	GPIO_InitStructure.Mode  = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Pull  = GPIO_NOPULL;
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET); 
}

void powerOn_LoRa(void){
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET); }

void powerOff_LoRa(void){
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET); 
}

void enter_StopMode(void){
	
}

