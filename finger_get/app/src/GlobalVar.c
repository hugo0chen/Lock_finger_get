#include "GlobalVar.h"
#include "schd.h"
#include "stm32l0xx_hal.h"

/*
 SysTick����ʱ�䣬���ڼ���ʱ������ÿ10ms����һ�Σ�
 */
__IO uint32_t _localTimeTick_ms = 0;
#define SYSTEMTICK_PERIOD_MS  10

void SysTick_Handler(void) {
	_localTimeTick_ms += SYSTEMTICK_PERIOD_MS;
	HAL_IncTick();	
	// ���ö�ʱ������
	Schd_Run(_localTimeTick_ms);
}

uint32_t local_ticktime() {
	return _localTimeTick_ms;
}

uint8_t timeout(uint32_t last_time, uint32_t ms) {
	return ((_localTimeTick_ms - last_time) > ms);
}

/*
 @brief ��ʱ
 */
void delay_us(uint32_t nus) {
	unsigned long delay_time = nus * 12;
	while (delay_time--)
		;
}

/*
 @brief ι��
 */
void watchdog(void) {

	//Feed Dog
	IWDG->KR = 0XAAAA;
}
