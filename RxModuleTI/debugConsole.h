/*
 * debugConsole.h
 *
 *  Created on: 20 lis 2016
 *      Author: igbt6
 */

#ifndef DEBUGCONSOLE_H_
#define DEBUGCONSOLE_H_

#include <stdbool.h>

void debugConsoleInit(void);

void debugConsolePrintf(const char *pcString, ...);

void debugCommandReceived();


#endif /* DEBUGCONSOLE_H_ */
