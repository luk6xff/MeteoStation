/*
 * esp8266.c
 *
 *  Created on: 20 lis 2016
 *      Author: igbt6
 */


#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"

#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/timer.h"
#include "driverlib/uart.h"

#include "esp8266.h"
#include "debugConsole.h"

//! - UART5 peripheral
//! - GPIO Port E peripheral (for UART5 pins)
//! - UART1RX - PE4
//! - UART1TX - PE5

#define TX_BUF_SIZE 64
#define RX_BUF_SIZE 256

static volatile uint32_t ms_counter= 0;
static volatile uint16_t rxBufferCounter= 0;
static volatile bool rxDataAvailable = false;
static uint8_t rxBuffer[RX_BUF_SIZE];
static uint8_t txBuffer[TX_BUF_SIZE];

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
	esp8266TimerInit();
}


//*****************************************************************************
//
//Buffer helpers
//
//*****************************************************************************
static void esp8266ResetUartRxBuffer(void)
{
	rxBufferCounter = 0;
	memset((void*)rxBuffer, '\0', RX_BUF_SIZE);
}

static bool esp8266isDataInRxBufferAvaialble(void)
{
	return (rxBufferCounter > 0) && rxDataAvailable;
}

static void esp8266ResetUartTxBuffer(void)
{
	memset((void*)txBuffer, '\0', TX_BUF_SIZE);
}

static void esp8266SetUartTxBuffer(const char* strToCopy)
{
	memcpy((void*)txBuffer, (void*)strToCopy, TX_BUF_SIZE);
}

static bool esp8266SearchForResponseString(const char* resp)
{
	bool res = false;
	bool charFound = false;
	uint16_t length = 0;
	uint16_t expectedLength = strlen(resp);
	for(uint16_t i = 0; i<RX_BUF_SIZE && rxBuffer[i] != '\0' ; ++i)
	{

		if(rxBuffer[i] == *resp)
		{
			resp++;
			if(charFound)
			{
				length++;
			}
			charFound = true;
		}
		else
		{
			resp-=length;
			length=0;
			charFound = false;
		}

		if(expectedLength == length)
		{
			res = true;
			break;
		}

	}
	return res;
}

static bool esp8266WaitForResponse(const char* resp, uint16_t msTimeout)
{
	bool res = false;
	uint32_t startTime = esp8266Milis();

	while((esp8266Milis() - startTime) < msTimeout)
	{
		if(rxDataAvailable)
		{
			if(esp8266SearchForResponseString(resp))
			{
				res = true;
				break;
			}
		}
	}
	rxDataAvailable = false;
	return res;
}

//*****************************************************************************
//
// Send a string to the UART.
//
//*****************************************************************************
static void esp8266UartSend(const char* dataBuffer)
{

	while (UARTBusy(UART5_BASE));
	while (*dataBuffer != '\0')
	{
		UARTCharPutNonBlocking(UART5_BASE, *dataBuffer++);
	}
}

static void esp8266SendATCommand(const char* cmd)
{
	esp8266UartSend(cmd);
	esp8266UartSend((const char*)"\r\n"); //CR LF
}


//AT Commands implementation
bool esp8266CommandAT(void)
{
	esp8266SendATCommand("AT");
	return esp8266WaitForResponse("OK", 2000);
}

bool esp8266CommandRST(void)
{
	esp8266SendATCommand("AT");
	return true;
}



// Uart5 interrupt handler
void Esp8266Uart5IntHandler(void)
{
	unsigned long status;
	char recvChr;
	// Get the interrrupt status.
	status = UARTIntStatus(UART5_BASE, true);
	UARTIntClear(UART5_BASE, status);
	esp8266ResetUartRxBuffer();
	//loop through all data in the fifo
	while (UARTCharsAvail(UART5_BASE))
	{
		recvChr = UARTCharGetNonBlocking(UART5_BASE);
		if(recvChr == '\0')
		{
			continue;
		}
		else
		{
			rxBuffer[rxBufferCounter++ % RX_BUF_SIZE] = recvChr;
		}
		//debugConsolePrintf("%c", UARTCharGetNonBlocking(UART5_BASE));
	}
	rxDataAvailable = true;
}

//Timer1A interrupt handler
void Esp8266Timer1AIntHandler(void)
{
	TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
	++ms_counter;
}
