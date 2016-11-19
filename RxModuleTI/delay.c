/*
 * delay.c
 *
 *  Created on: 19 lis 2016
 *      Author: igbt6
 */

#include "delay.h"


void delay_ms(uint32_t ms)
{
	SysCtlDelay(ms * (SysCtlClockGet() / (3 * 1000)));
}
