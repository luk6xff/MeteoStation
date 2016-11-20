/*
 * delay.c
 *
 *  Created on: 19 lis 2016
 *      Author: igbt6
 */


#include "inc/hw_types.h"
#include "inc/hw_memmap.h"

#include "driverlib/sysctl.h"

#include "delay.h"


void delay_ms(uint32_t ms)
{
	SysCtlDelay(ms * (SysCtlClockGet() / (3 * 1000)));
}
