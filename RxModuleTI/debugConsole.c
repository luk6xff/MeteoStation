/*
 * debugConsole.c
 *
 *  Created on: 20 lis 2016
 *      Author: igbt6
 */
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

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
static bool debugCommandDispatcher(const char* msg, int msgLen);
static bool debugCommandDispatcherESP8266(const char* cmdAttrs, int cmdAttrsLen);
static bool debugCommandDispatcherRFM23(const char* cmdAttrs, int cmdAttrsLen);


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
		UARTFlushRx();
		return;
	}

	if(bytesRxAvail >= DBG_CMD_MSG_MIN_LENGTH)
	{
		char msg[bytesRxAvail];
		UARTPeekBufferRX(msg);
		if(debugCommandDispatcher(msg, bytesRxAvail))
		{
			UARTFlushRx();
		}
	}
}


//Debug diagnostic message dispatching
typedef struct
{
	char* cmd;
	bool (*cmdDispatcher)(const char* cmdAttrs, int cmdAttrsLen);
}DebugCmdObject;

const DebugCmdObject debugCommands[DBG_CMDS_NUM] = {
		{ "ESP", debugCommandDispatcherESP8266 },
		{ "RFM", debugCommandDispatcherRFM23 }
};

static bool debugCommandDispatcherESP8266(const char* cmdAttrs, int cmdAttrsLen)
{
	const char* cmds[] = {
			"RAW"
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
	const char* delim ="_|"; // TODO should be passed ?
	bool resp = false;
	char* cmd = NULL;
	for (int i = 0; i < sizeof(cmds); ++i)
	{
		if(strncmp(cmds[i], cmdAttrs, strlen(cmds[i])) == 0)
		{
			cmd = (char*)cmds[i];
			resp = true;
			break;
		}
	}

	if(!resp)
	{
		return false;
	}

    int tokensNum = 0;
    char** tokens;
	char* token = strtok((char*)cmdAttrs, delim); //first token
	if(token != NULL)
	{
		tokens = (char**)malloc(sizeof(char*));
		if(tokens == NULL)
		{
			return false;
		}
		*tokens = strdup(token);
		tokensNum++;
		while ((token - cmdAttrs + strlen(token)) < cmdAttrsLen)
		{
			debugConsolePrintf("TOKEN: %s\n\r",token);
			token = strtok (NULL, delim);
			if(token != NULL)
			{
				char** moreTokens = (char**)realloc(tokens, sizeof(char*)*(tokensNum+1));
				if(moreTokens == NULL)
				{
					goto freeAll;
				}
				moreTokens[tokensNum++] = strdup(token);
				tokens = moreTokens;
			}
			else
			{
				break;
			}
		}
	}

	if(strcmp(cmd, "RAW") == 0)
	{
		if(tokensNum == 3)
		{
			if(esp8266CommandRAW(tokens[1], tokens[2]))
			{
				debugConsolePrintf("esp8266CommandRAW %s success\n\r", tokens[0] );
			}
			else
			{
				debugConsolePrintf("esp8266CommandRAW %s failed\n\r", tokens[0]);
			}
		}
	}

	else if(strcmp(cmd, "AT") == 0)
	{
		if(esp8266CommandAT())
		{
			debugConsolePrintf("esp8266CommandAT success\n\r");
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
			debugConsolePrintf("esp8266CommandRST success\n\r");
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
		if(tokensNum == 2)
		{
			Esp8266WifiMode mode = atoi(tokens[1]);
			if(esp8266CommandCWMODE(mode))
			{
				debugConsolePrintf("esp8266CommandCWMODE %s success \n\r", tokens[1]);
			}
			else
			{
				debugConsolePrintf("esp8266CommandCWMODE %s fail \n\r", tokens[1]);
			}
		}
	}

	else if(strcmp(cmd, "CIPMUX") == 0)
	{
		if(tokensNum == 2)
		{
			int enable = atoi(tokens[1]);
			if(esp8266CommandCIPMUX(enable))
			{
				debugConsolePrintf("esp8266CommandCIPMUX %s success \n\r", tokens[1]);
			}
			else
			{
				debugConsolePrintf("esp8266CommandCIPMUX %s fail \n\r", tokens[1]);
			}
		}
	}

	else if(strcmp(cmd, "CWLAP") == 0)
	{
		if(esp8266CommandCWLAP())
		{
			debugConsolePrintf("esp8266CommandCWLAP success \n\r");
		}
		else
		{
			debugConsolePrintf("esp8266CommandCWLAP fail \n\r");
		}
	}

	else if(strcmp(cmd, "CWJAP") == 0)
	{
		if(tokensNum == 3)
		{
			if(esp8266CommandCWJAP(tokens[1], tokens[2]))
			{
				debugConsolePrintf("esp8266CommandCWJAP %s success \n\r", tokens[1]);
			}
			else
			{
				debugConsolePrintf("esp8266CommandCWJAP %s fail \n\r", tokens[1]);
			}
		}
	}

	else if(strcmp(cmd, "CWQAP") == 0)
	{
		if(esp8266CommandCWQAP())
		{
			debugConsolePrintf("esp8266CommandCWQAP success \n\r");
		}
		else
		{
			debugConsolePrintf("esp8266CommandCWQAP fail \n\r");
		}
	}

	else if(strcmp(cmd, "CIFSR") == 0)
	{
		if(esp8266CommandCIFSR())
		{
			debugConsolePrintf("esp8266CommandCIFSR success \n\r");
		}
		else
		{
			debugConsolePrintf("esp8266CommandCIFSR fail \n\r");
		}
	}

	else if(strcmp(cmd, "CIPSTART") == 0)
	{
		if(tokensNum == 2)
		{
			if(esp8266CommandCIPSTART(tokens[1]))
			{
				debugConsolePrintf("esp8266CommandCIPSTART %s success \n\r", tokens[1]);
			}
			else
			{
				debugConsolePrintf("esp8266CommandCIPSTART %s fail \n\r", tokens[1]);
			}
		}
	}

	else if(strcmp(cmd, "CIPCLOSE") == 0)
	{
		if(esp8266CommandCIPCLOSE())
		{
			debugConsolePrintf("esp8266CommandCIPCLOSE success \n\r");
		}
		else
		{
			debugConsolePrintf("esp8266CommandCIPCLOSE fail \n\r");
		}
	}

	else if(strcmp(cmd, "CIPSEND") == 0)
	{
		if(tokensNum == 2)
		{
			if(esp8266CommandCIPSEND(tokens[1]))
			{
				debugConsolePrintf("esp8266CommandCIPSTART %s success \n\r", tokens[1]);
			}
			else
			{
				debugConsolePrintf("esp8266CommandCIPSTART %s fail \n\r", tokens[1]);
			}
		}
	}

	freeAll:
	for(int i = 0; i < tokensNum; ++i)
	{
	    free(tokens[i]);
	}
    free(tokens);
	return resp;
}

static bool debugCommandDispatcherRFM23(const char* cmdAttrs, int cmdAttrsLen)
{
	return false;
}


//
//@brief All debug commands dispatcher,
//parses commands created with pattern: "C-ESP_XXX_XXX_XXX|\r\n" where XXX are attributes of the given command
//@note "C-" is a command header
//@note "ESP" is a given command (test module)
//@note "_" is a parameters delimeter
//@note "|\r\n" is a last delimeter of the command showing command's end
//
static bool debugCommandDispatcher(const char* msg, int msgLen)
{
	const uint8_t cmdLength = 3;
	const char header[] = "C-";
	const char delim[] = "_";
	const char endDelimeter[] = "|";
	const uint8_t headerLength = strlen(header);
	const uint8_t delimLength = strlen(delim);
	const uint8_t endDelimLength = strlen(endDelimeter);

	char cmd[cmdLength];
	bool ret = false;

	//search for header in the payload
	int headerIdx = -1;
	for(int i = 0; i < msgLen; ++i)
	{
		if(msg[i] == header[0] && i < msgLen-headerLength+1)
		{
			if(strncmp(&msg[i], header, headerLength) == 0)
			{
				headerIdx = i;
				break;
			}
		}
	}
	if(headerIdx == -1)
	{
		return true; // just to signalize invalid msg, further buffer flushing
	}
	//search for delimeter in the payload
	int delimIdx = -1;
	for(int i = headerIdx; i < msgLen; ++i)
	{
		if(msg[i] == endDelimeter[0] && i < msgLen-endDelimLength+1)
		{
			if(strncmp(&msg[i], endDelimeter, endDelimLength) == 0)
			{
				delimIdx = i;
				break;
			}
		}
	}

	if(delimIdx != -1)
	{
		if(strncmp(header, msg + headerIdx, headerLength) == 0)
		{
			memcpy(cmd, msg+headerIdx+headerLength, cmdLength);

			for(uint8_t i = 0; i < DBG_CMDS_NUM; ++i)
			{
				if(strncmp(cmd, debugCommands[i].cmd, cmdLength) == 0)
				{
					int cmdAttrsLen = delimIdx - headerIdx - headerLength - cmdLength - delimLength + endDelimLength;
					ret = (*debugCommands[i].cmdDispatcher)(msg + (headerIdx + headerLength + cmdLength + delimLength), cmdAttrsLen);
				}
			}
		}
	}
	if(headerIdx != -1 && delimIdx != -1 && ret == false)
	{
		ret = true; //flush the invalid msg
	}
	return ret;
}

