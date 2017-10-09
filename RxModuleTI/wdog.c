/*
 * wdog.c
 *
 *  Created on: 13 wrz 2017
 *      Author: igbt6
 */
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"

#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/watchdog.h"

#include "wdog.h"

#define WATCHDOG_PERIOD_SEC  20  //20s

//@brief Initializes watchdog 0
void watchdog_init()
{
	///Initialize Watchdog0
	MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_WDOG0);
	MAP_IntEnable(INT_WATCHDOG);
	if(MAP_WatchdogLockState(WATCHDOG0_BASE) == true)
	{
		MAP_WatchdogUnlock(WATCHDOG0_BASE);
	}
	// Enable Watchdog Interrupts
	MAP_WatchdogIntEnable(WATCHDOG0_BASE);
	MAP_WatchdogIntTypeSet(WATCHDOG0_BASE, WATCHDOG_INT_TYPE_INT);

	// Set Watchdog Properties
	MAP_WatchdogStallEnable(WATCHDOG0_BASE);
	MAP_WatchdogReloadSet(WATCHDOG0_BASE, MAP_SysCtlClockGet() * WATCHDOG_PERIOD_SEC);
	MAP_WatchdogResetEnable(WATCHDOG0_BASE);
	MAP_WatchdogEnable(WATCHDOG0_BASE);
	MAP_WatchdogLock(WATCHDOG0_BASE);
}


void watchdog_reset()
{
	MAP_WatchdogUnlock(WATCHDOG0_BASE);
	MAP_WatchdogReloadSet(WATCHDOG0_BASE, MAP_SysCtlClockGet() * WATCHDOG_PERIOD_SEC);
	MAP_WatchdogLock(WATCHDOG0_BASE);
}


//
//@brief Watchdog ISR handler
//
void WatchdogHandler(void)
{
	MAP_WatchdogIntClear(WATCHDOG0_BASE);
	MAP_SysCtlReset();
}
