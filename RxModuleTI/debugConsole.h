/*
 * debugConsole.h
 *
 *  Created on: 20 lis 2016
 *      Author: igbt6
 */

#ifndef DEBUGCONSOLE_H_
#define DEBUGCONSOLE_H_

#include "system.h"

void debugConsoleInit(void);

void debugConsolePrintf(const char *pcString, ...);

void debugCommandReceived();

void DEBUG(bool enable, const char* moduleName, const char* fmt, ...);

#endif /* DEBUGCONSOLE_H_ */
