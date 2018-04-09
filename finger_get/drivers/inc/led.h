#ifndef __LED_H__
#define __LED_H__

#include "stm32l0xx.h"
#include "stm32l0xx_hal.h"

/*
 @brief LED��״̬
 */
enum LED_STATE {
	LEDON, 
	LEDOFF
};

/*
 @brief LED���
 */
enum LED_NO {
	LED_NO_0 = 0,
	LED_NO_1 = 1,
	BUZZER = 2
};

/*
 @brief LED���������
 */
#define LED_MAX_SIZE (3)

/*
 @brief LED����
 */
struct LedDefine {
	enum LED_NO led_no;
	// PIN_GROUP��GPIOA ...��
	GPIO_TypeDef* pin_group;
	// PIN��GPIO_Pin_1 ...��
	uint16_t pin;
	// LEDON | LEDOFF
	enum LED_STATE defaultstatus;
};

void init_led(void);

/*
 @brief ����LED
 @param led_no LED���
 */
uint8_t LED_ON(enum LED_NO led_no);

/*
 @brief Ϩ��LED
 @param led_no LED���
 */
uint8_t LED_OFF(enum LED_NO led_no);

uint8_t LED_TOGGLE(enum LED_NO led_no);

void buzzer(uint8_t freq, uint16_t time);
#endif
