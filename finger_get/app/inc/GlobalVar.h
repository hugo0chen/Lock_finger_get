#ifndef __GLOBAL_VAR_H__
#define __GLOBAL_VAR_H__
#include "stm32l0xx.h"

#define clrbit(reg,bit)   reg &= ~(bit)	/*��Ĵ�����ĳ1����λ*/
#define bitclr(reg,bit)   reg &= ~(bit)	/*��Ĵ�����ĳ1����λ*/

#define setbit(reg,bit)   reg |=  (bit)	/*���üĴ�����ĳ1����λ*/
#define bitset(reg,bit)   reg |=  (bit)	/*���üĴ�����ĳ1����λ*/
#define cplbit(reg,bit)   reg ^=  (bit)	/*�ԼĴ�����ĳ1����λȡ��*/
#define bitcpl(reg,bit)   reg ^=  (bit)	/*�ԼĴ�����ĳ1����λȡ��*/

#define testbit(reg,bit) (reg&(bit))
#define bittest(reg,bit) (reg&(bit))

#define regcpl(reg)		 reg = ~reg
#define regclr(reg)		 reg = 0


#define ARRAY_SIZE(array, type) (sizeof(array)/sizeof(type))
#define STR_EQ(a,b) (strcmp((a),(b))==0)

typedef enum { False=0, True=1 }BOOL;
/*
  @brief ��ȡ��ǰϵͳ��������ǰ�ĺ���
*/
uint32_t local_ticktime(void);

/*
  @brief ���������ʱ���Ƿ�ʱ
*/
uint8_t timeout(uint32_t last_time, uint32_t ms);

/*
 @brief ��ʱ
*/
void delay_us(uint32_t nus);

/*
 @brief ι��
*/
void watchdog(void);


#endif	/* __GLOBAL_VAR_H__ */

