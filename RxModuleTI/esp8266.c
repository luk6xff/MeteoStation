/*
 * esp8266.c
 *
 *  Created on: 20 lis 2016
 *      Author: igbt6
 */


#include <stdint.h>
#include <stdbool.h>

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"

#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/timer.h"
#include "driverlib/uart.h"

#include "esp8266.h"

//! - UART5 peripheral
//! - GPIO Port E peripheral (for UART5 pins)
//! - UART1RX - PE4
//! - UART1TX - PE5


static volatile uint32_t ms_counter= 0;

//sets Timer for ESP8266
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


//Fires up/down the timer
static void esp8266TimerEnable(bool enable)
{
	if(enable)
	{
		TimerEnable(TIMER1_BASE, TIMER_A);
		return;
	}
    TimerDisable(TIMER1_BASE, TIMER_A);
}

static uint32_t esp8266Milis(void)
{
	return ms_counter;
}

//Configures Timer1A as a 32-bit periodic timer
static void esp8266TimerInit()
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);

    TimerConfigure(TIMER1_BASE, TIMER_CFG_32_BIT_PER_UP);
    // Set the Timer1A load value to 1ms.
    TimerLoadSet(TIMER1_BASE, TIMER_A, SysCtlClockGet() / 1000); //1 [ms]
    IntMasterEnable();

    // Configure the Timer1A interrupt for timer timeout.
    TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);

    // Enable the Timer0A interrupt on the processor (NVIC).
    IntEnable(INT_TIMER1A);

    ms_counter = 0;

    // Enable Timer1A.
    esp8266TimerEnable(true);
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


void ESP8266Timer1AIntHandler(void)
{
	TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
	++ms_counter;
}
