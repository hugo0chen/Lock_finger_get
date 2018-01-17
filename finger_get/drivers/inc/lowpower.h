#ifndef _LOWPOWER_H_
#define _LOWPOWER_H_
#include "stm32l0xx.h"

void init_PowerControl_Configuration(void);
void powerOn_LoRa(void);
void powerOff_LoRa(void);

#endif
