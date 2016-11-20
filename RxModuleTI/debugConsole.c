/*
 * debugConsole.c
 *
 *  Created on: 20 lis 2016
 *      Author: igbt6
 */

#include "debugConsole.h"
#include "utils/uartstdio.h"

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"

#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"



void debugConsoleInit(void) {
	// Enable GPIO port A which is used for UART0 pins.
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

	GPIOPinConfigure(GPIO_PA0_U0RX);
	GPIOPinConfigure(GPIO_PA1_U0TX);

	GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	// Initialize the UART for console I/O.
	UARTStdioInit(0);
}


void debugConsolePrintf(const char *pcString, ...)
{
	UARTprintf(pcString);
}
