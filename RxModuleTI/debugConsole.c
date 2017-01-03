/*
 * debugConsole.c
 *
 *  Created on: 20 lis 2016
 *      Author: igbt6
 */
#include <stdint.h>
#include <string.h>

#include "debugConsole.h"
#include "utils/uartstdio.h"

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"

#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"




static const uint8_t debugEnable = 1;

#define DBG_CMD_MSG_MIN_LENGTH 7   // min 5 chars for command eg. C-ESP_AT
#define DBG_CMDS_NUM 2 			   // Number of debug commands
static void debugCommandDispatcher(const char* msg);
static bool debugCommandDispatcherESP8266(const char* params);
static bool debugCommandDispatcherRFM23(const char* params);


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
	if(debugEnable)
	{
		UARTprintf(pcString);
	}
}

bool debugCommandReceived()
{
	uint8_t bytesRxAvail = UARTRxBytesAvail();
	if(bytesRxAvail >= DBG_CMD_MSG_MIN_LENGTH)
	{
		char msg[bytesRxAvail];
		UARTgets(msg, bytesRxAvail);
		debugCommandDispatcher(msg);
		return true;
	}
	return false;
}


//Debug diagnostic message dispatching
typedef struct
{
	char* cmd;
	bool (*cmdDispatcher)(const char* params);
}DebugCmdObject;

const DebugCmdObject debugCommands[DBG_CMDS_NUM] = {
		{ "ESP", debugCommandDispatcherESP8266 },
		{ "RFM", debugCommandDispatcherRFM23 }
};

static bool debugCommandDispatcherESP8266(const char* params)
{
	const char* func[] = {
			"AT",
			"RST",
			"GMR",
			"CWMODE",
			"CIPMUX",
			"CWLAP",
			"CWJAP",
			"CWQAP",
			"CWSAP",
			"CIFSR",
			"CIPSTART",
			"CIPCLOSE",
			"CIPSEND"
	};
	return false;
}

static bool debugCommandDispatcherRFM23(const char* params)
{
	return false;
}

static void debugCommandDispatcher(const char* msg)
{
	const uint8_t headerLength = 2;
	const uint8_t cmdLength = 3;
	const char header[] = "C-";
	char cmd[cmdLength];

	if(strncmp(header, msg, headerLength) == 0)
	{
		memcpy(cmd, msg+2, cmdLength);

		for(uint8_t i = 0; i < DBG_CMDS_NUM; ++i)
		{
			if(strncmp(cmd, debugCommands[i].cmd, cmdLength) == 0)
			{
				(*debugCommands[i].cmdDispatcher)(msg + (headerLength + cmdLength + 1));
			}
		}
	}
}

