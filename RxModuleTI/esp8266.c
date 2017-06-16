/*
 * esp8266.c
 *
 *  Created on: 20 lis 2016
 *      Author: igbt6
 */


#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"

#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/timer.h"
#include "driverlib/uart.h"

#include "system.h"
#include "esp8266.h"


#define ESP8266_DEBUG_ENABLE 1
#if ESP8266_DEBUG_ENABLE
	#include "debugConsole.h"
#endif
#define ESP8266_DEBUG(str) \
		do \
		{  \
			if (ESP8266_DEBUG_ENABLE) \
				debugConsolePrintf(str); \
		}while(0);


//! - UART5 peripheral
//! - GPIO Port E peripheral (for UART5 pins)
//! - UART1RX - PE4
//! - UART1TX - PE5


//Function prototypes
static bool esp8266WaitForResponse(const char* resp, uint16_t msTimeout);
static void esp8266UartSend(const char* dataBuffer, const uint16_t dataBufferLen);
static void esp8266SendATCommand(const char* cmd);

//data types

static volatile uint32_t ms_counter= 0;
static volatile uint16_t rxBufferCounter= 0;
static volatile bool rxDataAvailable = false;
static uint8_t rxBuffer[ESP8266_RX_BUF_SIZE];
static uint8_t txBuffer[ESP8266_TX_BUF_SIZE];

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

static void esp8266Delay(uint32_t msDelay)
{
	uint32_t start = esp8266Milis();
	while((esp8266Milis() - start) < msDelay)
	{
	}
}

//Configures Timer1A as a 32-bit periodic timer
static void esp8266TimerInit()
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);

    TimerConfigure(TIMER1_BASE, TIMER_CFG_32_BIT_PER_UP);
    // Set the Timer1A load value to 1ms.
    TimerLoadSet(TIMER1_BASE, TIMER_A, SysCtlClockGet() / 1000); //1 [ms]

    // Configure the Timer1A interrupt for timer timeout.
    TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);

    // Enable the Timer1A interrupt on the processor (NVIC).
    IntEnable(INT_TIMER1A);

    ms_counter = 0;

    // Enable Timer1A.
    esp8266TimerEnable(true);
}


void esp8266Init()
{
	esp8266UartSetup();
	esp8266TimerInit();
	//enable all
	ENABLE_ALL_INTERRUPTS();
}


uint8_t* esp8266GetRxBuffer(void)
{
	return rxBuffer;
}

//*****************************************************************************
//
//Buffer helpers
//
//*****************************************************************************
static void esp8266ResetUartRxBuffer(void)
{
	rxBufferCounter = 0;
	memset((void*)rxBuffer, '\0', ESP8266_RX_BUF_SIZE);
}

static bool esp8266isDataInRxBufferAvaialble(void)
{
	return (rxBufferCounter > 0) && rxDataAvailable;
}

static void esp8266ResetUartTxBuffer(void)
{
	memset((void*)txBuffer, '\0', ESP8266_TX_BUF_SIZE);
}

static void esp8266SetUartTxBuffer(const char* strToCopy)
{
	esp8266ResetUartTxBuffer();
	memcpy((void*)txBuffer, (void*)strToCopy, strlen(strToCopy));
}

static bool esp8266SearchForResponseString(const char* resp)
{
	bool res = false;
	uint16_t length = 0;
	uint16_t expectedLength = strlen(resp);
	for(uint16_t i = 0; i<ESP8266_RX_BUF_SIZE && rxBuffer[i] != '\0' ; ++i)
	{
		if(rxBuffer[i] == *resp)
		{
			resp++;
			length++;
		}
		else
		{
			resp-=length;
			length=0;
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
static void esp8266UartSend(const char* dataBuffer, const uint16_t dataBufferLen)
{

	while (UARTBusy(UART5_BASE));
	uint16_t dataLen = dataBufferLen;
	if (dataLen == 0)
	{
		while (*dataBuffer != '\0')
		{
			UARTCharPut(UART5_BASE, *dataBuffer++);
		}
	}
	else
	{
		while (dataLen > 0)
		{
			UARTCharPut(UART5_BASE, *dataBuffer++);
			dataLen--;
		}
	}
}

static void esp8266SendATCommand(const char* cmd)
{
	DISABLE_ALL_INTERRUPTS();
	esp8266ResetUartRxBuffer();
	esp8266UartSend(cmd, 0);
	esp8266UartSend((const char*)"\r\n", 0); //CR LF
	ENABLE_ALL_INTERRUPTS();
}


//AT Commands implementation
bool esp8266CommandRAW(const char* rawCmd, const char*respStr)
{
	esp8266SendATCommand(rawCmd);
	return esp8266WaitForResponse(respStr, 8000);
}

bool esp8266CommandAT(void)
{
	esp8266SendATCommand("AT");
	return esp8266WaitForResponse("OK", 2000);
}


bool esp8266CommandRST(void)
{
	esp8266SendATCommand("AT+RST");
	return esp8266WaitForResponse("OK", 2000);
}


bool esp8266CommandGMR(void)
{
	esp8266SendATCommand("AT+GMR");
	return esp8266WaitForResponse("OK", 2000);
}


bool esp8266CommandCWMODE(Esp8266WifiMode mode)
{
	if(mode >=ESP8266_MODE_INVALID)
	{
		return false;
	}
	esp8266ResetUartTxBuffer();
	sprintf((char*)txBuffer, "AT+CWMODE=%d", mode);
	esp8266SendATCommand((char*)txBuffer);
	return esp8266WaitForResponse("OK", 2000);
}


bool esp8266CommandCIPMUX(uint8_t enable)
{
	if(enable > 1)
	{
		return false;
	}
	esp8266ResetUartTxBuffer();
	sprintf((char*)txBuffer, "AT+CIPMUX=%d", enable);
	esp8266SendATCommand((char*)txBuffer);
	return esp8266WaitForResponse("OK", 2000);
}


bool esp8266CommandCWLAP()
{
	esp8266SendATCommand("AT+CWLAP");
	return esp8266WaitForResponse("OK", 9000);
}


bool esp8266CommandCWJAP(const char* ssid, const char* pass)
{
	esp8266ResetUartTxBuffer();
	sprintf((char*)txBuffer, "AT+CWJAP=\"%s\",\"%s\"", ssid, pass);
	esp8266SendATCommand((char*)txBuffer);
	return esp8266WaitForResponse("OK", 10000);
}


bool esp8266CommandCWQAP(void)
{
	esp8266SendATCommand("AT+CWQAP");
	return esp8266WaitForResponse("OK", 8000);
}


bool esp8266CommandCWSAP(const char* ssid, const char* password, uint8_t channel, uint8_t encryptMode)
{
	esp8266ResetUartTxBuffer();
	sprintf((char*)txBuffer, "AT+CWSAP=\"%s\",\"%s\",%d,%d", ssid, password, channel, encryptMode);
	esp8266SendATCommand((char*)txBuffer);
	return esp8266WaitForResponse("OK", 5000);
}


bool esp8266CommandCIFSR(void)
{
	esp8266SendATCommand("AT+CIFSR");
	return esp8266WaitForResponse("OK", 5000);
}

bool esp8266CommandCIPSTATUS(void)
{
	esp8266SendATCommand("AT+CIPSTATUS");
	return esp8266WaitForResponse("OK", 5000);
}


bool esp8266CommandCIPSTART(Esp8266Protocol proto, const char* ipAddr, uint16_t portNum)
{
	if (proto >= ESP8266_PROTOCOL_NUM)
	{
		return false;
	}
	char protocol[4] = {'\0'};
	switch (proto)
	{
		case ESP8266_PROTOCOL_UDP:
			memcpy(protocol, "UDP", 3);
			break;
		case ESP8266_PROTOCOL_TCP:
		default:
			memcpy(protocol, "TCP", 3);
			break;
	}
	esp8266ResetUartTxBuffer();
	sprintf((char*)txBuffer, "AT+CIPSTART=\"%s\",\"%s\",%d",protocol, ipAddr, portNum);
	esp8266SendATCommand((char*)txBuffer);
	return esp8266WaitForResponse("OK", 8000);
}


bool esp8266CommandCIPCLOSE()
{
	esp8266SendATCommand("AT+CIPCLOSE");
	return esp8266WaitForResponse("OK", 5000);
}


bool esp8266CommandCIPSEND(const char* packet, uint16_t packetLen)
{
	esp8266ResetUartTxBuffer();
	if (packetLen == 0)
	{
		sprintf((char*)txBuffer, "AT+CIPSEND=%d", strlen(packet));
	}
	else
	{
		sprintf((char*)txBuffer, "AT+CIPSEND=%d", packetLen);
	}
	esp8266SendATCommand((char*)txBuffer);
	if(!esp8266WaitForResponse(">", 4000))
	{
		return false;
	}
	//DISABLE_ALL_INTERRUPTS();
	esp8266ResetUartRxBuffer();
	esp8266UartSend(packet, packetLen);
	//ENABLE_ALL_INTERRUPTS();

	return esp8266WaitForResponse("SEND OK", 4000);
}

//
//interrupt handlers
//

// Uart5 interrupt handler
void Esp8266Uart5IntHandler(void)
{
	unsigned long status;
	char recvChr;
	// Get the interrrupt status.
	status = UARTIntStatus(UART5_BASE, true);
	UARTIntClear(UART5_BASE, status);
	//loop through all data in the fifo
	while (UARTCharsAvail(UART5_BASE))
	{
		recvChr = UARTCharGetNonBlocking(UART5_BASE);
		rxBuffer[rxBufferCounter++ % ESP8266_RX_BUF_SIZE] = recvChr;
	}
	rxDataAvailable = true;
}

//Timer1A interrupt handler
void Esp8266Timer1AIntHandler(void)
{
	TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
	++ms_counter;
}
