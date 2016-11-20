/*
 * esp8266.c
 *
 *  Created on: 20 lis 2016
 *      Author: igbt6
 */


#include <stdint.h>

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"

#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/uart.h"

#include "esp8266.h"

//! - UART5 peripheral
//! - GPIO Port E peripheral (for UART5 pins)
//! - UART1RX - PE4
//! - UART1TX - PE5

static void esp8266UartSetup(void) {
	//
	// Enable the peripherals used by the UART5
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART5);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);

	// Set GPIO PE4 and PE5 as UART pins.
	GPIOPinConfigure(GPIO_PE4_U5RX);
	GPIOPinConfigure(GPIO_PE5_U5TX);
	GPIOPinTypeUART(GPIO_PORTE_BASE, GPIO_PIN_4 | GPIO_PIN_5);


	// Configure the UART for 115200, 8-N-1 operation.
	UARTConfigSetExpClk(UART5_BASE, SysCtlClockGet(), 115200,
			(UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
			UART_CONFIG_PAR_NONE));

	// Enable the  UART5 RX interrupts.
	IntMasterEnable();
	IntEnable(INT_UART5);
	UARTIntEnable(UART5_BASE, UART_INT_RX | UART_INT_RT);

}


void esp8266Init()
{
	esp8266UartSetup();
}


//*****************************************************************************
//
// Send a string to the UART.
//
//*****************************************************************************
static void esp8266UartSend(const uint8_t* dataBuffer)
{

	while (UARTBusy(UART5_BASE));
	while (*dataBuffer != '\0')
	{
		UARTCharPutNonBlocking(UART5_BASE, *dataBuffer++);
	}
}

void esp8266SendATCommand(const uint8_t* cmd)
{
	esp8266UartSend(cmd);
	esp8266UartSend((const uint8_t*)"\r\n"); //CR LF
}


void UART5IntHandler(void) {

	unsigned long ulStatus;

	// Get the interrrupt status.
	ulStatus = UARTIntStatus(UART5_BASE, true);

	// Clear the asserted interrupts.
	UARTIntClear(UART5_BASE, ulStatus);

	// Loop while there are characters in the receive FIFO.
	while (UARTCharsAvail(UART5_BASE)) {
		// Read the next character from the UART and write it to the console.
		//UARTCharPutNonBlocking(UART5_BASE, UARTCharGetNonBlocking(UART5_BASE));
		//UARTprintf("%c", UARTCharGetNonBlocking(UART5_BASE));
	}
}
