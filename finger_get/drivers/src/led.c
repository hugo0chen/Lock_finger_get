#include "led.h"
#include <string.h>
#include "delay.h"

struct LedDefine _leds_defines[LED_MAX_SIZE]; 

static uint8_t LED_Configuration(const struct LedDefine* leds, uint8_t len)
{
    uint8_t l;
    GPIO_InitTypeDef	GPIO_InitStructure;
		if(len > LED_MAX_SIZE) 
			return 1;
    memset( (void*)&_leds_defines[0], 0, sizeof(_leds_defines) );
	
    for (l=0;l<len;++l){
        const struct LedDefine def = leds[l];
        _leds_defines[def.led_no] = def;
        /* Enable the GPIOA Clock */
				__HAL_RCC_GPIOB_CLK_ENABLE();

				/* Configure the GPIO_LED pin */
				GPIO_InitStructure.Pin 	 = def.pin;
				GPIO_InitStructure.Mode  = GPIO_MODE_OUTPUT_PP;
				GPIO_InitStructure.Pull  = GPIO_NOPULL;
				GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
				
				HAL_GPIO_Init(def.pin_group, &GPIO_InitStructure);

				/* Set PIN to switch off the LED */
				HAL_GPIO_WritePin(def.pin_group, def.pin, GPIO_PIN_SET); 
        if (def.defaultstatus == LEDON){
            LED_ON(def.led_no);
        }
        else{
            LED_OFF(def.led_no);
        }
    }
    return 0;
}
void init_led(void){
	struct LedDefine conf[] = {
		{LED_NO_0, GPIOB,	GPIO_PIN_6,	 LEDOFF},
		{LED_NO_1, GPIOB,	GPIO_PIN_7,  LEDOFF},
		{BUZZER,   GPIOB,	GPIO_PIN_3,  LEDON}
	};
	LED_Configuration(conf, sizeof(conf)/sizeof(struct LedDefine));
	
}
//Low enale = light LED
uint8_t LED_ON(enum LED_NO led_no){
    if (_leds_defines[led_no].pin!=0){
        HAL_GPIO_WritePin(_leds_defines[led_no].pin_group, _leds_defines[led_no].pin,GPIO_PIN_RESET);
        return 0;
    }
    return 1;
}

uint8_t LED_OFF(enum LED_NO led_no){
		if (_leds_defines[led_no].pin!=0){
        HAL_GPIO_WritePin(_leds_defines[led_no].pin_group, _leds_defines[led_no].pin, GPIO_PIN_SET);
        return 0;
    }
    return 1;
}

uint8_t LED_TOGGLE(enum LED_NO led_no){
		if (_leds_defines[led_no].pin != 0){
        HAL_GPIO_TogglePin(_leds_defines[led_no].pin_group, _leds_defines[led_no].pin);
        return 0;
    }
    return 1;
}

uint8_t BUZZER_ON(enum LED_NO buz){
	if (_leds_defines[buz].pin != 0){
		HAL_GPIO_WritePin(_leds_defines[buz].pin_group, _leds_defines[buz].pin, GPIO_PIN_SET);
	}
	return 1;
}
uint8_t BUZZER_OFF(enum LED_NO buz){
	if (_leds_defines[buz].pin != 0){
		HAL_GPIO_WritePin(_leds_defines[buz].pin_group, _leds_defines[buz].pin, GPIO_PIN_RESET);
	}
	return 1;
}

void buzzer(uint8_t freq, uint16_t time){
	uint8_t i;
	for(i = 0; i < freq; i++){
		BUZZER_ON(BUZZER);
		for(uint16_t j = 0; j < time; j++){
			Delay_1ms();
		}
		BUZZER_OFF(BUZZER);
		for(uint16_t j = 0; j < time; j++){
			Delay_1ms();
		}
	}
}


