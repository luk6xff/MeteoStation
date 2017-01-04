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

// debug Console Test Modules
#include "esp8266.h"




static const uint8_t debugEnable = 1;

#define DBG_CMD_MSG_MIN_LENGTH 7   // min 5 chars for command eg. C-ESP_AT
#define DBG_CMDS_NUM 2 			   // Number of debug commands
static bool debugCommandDispatcher(const char* msg);
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

void debugCommandReceived()
{
	uint8_t bytesRxAvail = UARTRxBytesAvail();

	if(UARTTxBytesFree() == 0)
	{
		return;
	}

	if(bytesRxAvail >= DBG_CMD_MSG_MIN_LENGTH)
	{
		char msg[bytesRxAvail];
		UARTPeekBufferRX(msg);
		if(debugCommandDispatcher(msg))
		{
			UARTFlushRx();
		}
	}
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
	const char* cmds[] = {
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
	bool resp = false;
	char* cmd = NULL;
	for (int i = 0; i < sizeof(cmds); ++i)
	{
		if(strncmp(cmds[i], params, strlen(cmds[i])) == 0)
		{
			cmd = cmds[i];
			resp = true;
			break;
		}
	}

	if(!resp)
	{
		return false;
	}

	if(strcmp(cmd, "AT") == 0)
	{
		if(esp8266CommandAT())
		{
			debugConsolePrintf("esp8266CommandAT succesfully sent\n\r");
		}
		else
		{
			debugConsolePrintf("esp8266CommandAT failed\n\r");
		}
	}

	else if(strcmp(cmd, "RST") == 0)
	{
		if(esp8266CommandRST())
		{
			debugConsolePrintf("esp8266CommandRST succesfully sent\n\r");
		}
		else
		{
			debugConsolePrintf("esp8266CommandRST failed\n\r");
		}
	}
	else if(strcmp(cmd, "GMR") == 0)
	{
		if(esp8266CommandGMR())
		{
			debugConsolePrintf("esp8266CommandGMR success \n\r");
		}
		else
		{
			debugConsolePrintf("esp8266CommandGMR fail \n\r");
		}
	}
	else if(strcmp(cmd, "CWMODE") == 0)
	{
		if(esp8266CommandRST())
		{
			debugConsolePrintf("esp8266CommandRST succesfully sent\n\r");
		}
		else
		{
			debugConsolePrintf("esp8266CommandRST failed\n\r");
		}
	}

	return resp;
}

static bool debugCommandDispatcherRFM23(const char* params)
{
	return false;
}

static bool debugCommandDispatcher(const char* msg)
{
	const uint8_t headerLength = 2;
	const uint8_t cmdLength = 3;
	const char header[] = "C-";
	char cmd[cmdLength];

	bool ret = false;

	if(strncmp(header, msg, headerLength) == 0)
	{
		memcpy(cmd, msg+2, cmdLength);

		for(uint8_t i = 0; i < DBG_CMDS_NUM; ++i)
		{
			if(strncmp(cmd, debugCommands[i].cmd, cmdLength) == 0)
			{
				ret = (*debugCommands[i].cmdDispatcher)(msg + (headerLength + cmdLength + 1));
			}
		}
	}
	return ret;
}

