#include "lowpower.h"
#include "stm32l0xx_hal.h"

//PA1  -- LORA POWER CONTROL, High valid
//PB12 -- Finger VCC control, High valid old PCB , office door lock
//PB9 --finger VCC control high valid  for home lock

struct PowerDefine power[2] = {
	{POWER_NO_0, GPIOA, GPIO_PIN_1, POWEROFF},
	{POWER_NO_1, GPIOB, GPIO_PIN_9, POWEROFF}
};

void init_PowerControl_Configuration(void){
	GPIO_InitTypeDef	GPIO_InitStructure;

	GPIO_InitStructure.Pin 	 = power[0].pin;
	GPIO_InitStructure.Mode  = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Pull  = GPIO_PULLUP;
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init(power[0].pin_group, &GPIO_InitStructure);
	HAL_GPIO_WritePin(power[0].pin_group, power[0].pin, GPIO_PIN_RESET); 
	
	GPIO_InitStructure.Pin 	 = power[1].pin;
	GPIO_InitStructure.Pull  = GPIO_PULLUP;
	HAL_GPIO_Init(power[1].pin_group, &GPIO_InitStructure);
	HAL_GPIO_WritePin(power[0].pin_group, power[0].pin, GPIO_PIN_RESET); 
}

void powerOn_LoRa(void){
	HAL_GPIO_WritePin(power[0].pin_group, power[0].pin, GPIO_PIN_SET);
	power[0].status = POWERON;
}

void powerOff_LoRa(void){
	HAL_GPIO_WritePin(power[0].pin_group, power[0].pin,  GPIO_PIN_RESET); 
	power[0].status = POWEROFF;
}

void powerOn_finger(void){
	HAL_GPIO_WritePin(power[1].pin_group, power[1].pin, GPIO_PIN_SET); 
	power[1].status = POWERON;
}

void powerOff_finger(void){
	HAL_GPIO_WritePin(power[1].pin_group, power[1].pin, GPIO_PIN_RESET); 
	power[1].status = POWEROFF;
}
