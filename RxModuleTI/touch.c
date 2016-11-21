/*
 * touch.c
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
#include "driverlib/timer.h"
#include "touch.h"
#include "ads7843.h"

#include "debugConsole.h"  //FOR DEBUG ONLY

#define TIMER_SCAN_PERIOD_MS 1000 //
//touch callback
static int32_t (*touchCallback)(uint32_t message, int32_t x, int32_t y);
static volatile uint32_t m_counter = 0; //increments every 1 [ms]

//private methods declarations
static void touchScreenTimerInit();
static void touchScreenTimerEnable(bool enable);
static void touchIntPinInit();
static void touchIntPinInterruptEnable(bool enable);

//Configures Timer0A as a 32-bit periodic timer
static void touchScreenTimerInit()
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

    TimerConfigure(TIMER0_BASE, TIMER_CFG_32_BIT_PER_UP);
    // Set the Timer0A load value to 1ms.
    TimerLoadSet(TIMER0_BASE, TIMER_A, SysCtlClockGet() / 1000); //1 [ms]
    IntMasterEnable();

    // Configure the Timer0A interrupt for timer timeout.
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    // Enable the Timer0A interrupt on the processor (NVIC).
    IntEnable(INT_TIMER0A);

    m_counter = 0;

    // Disable Timer0A.
    touchScreenTimerEnable(false);
}

//Fires up/down the timer
static void touchScreenTimerEnable(bool enable)
{

	if(enable)
	{
		TimerEnable(TIMER0_BASE, TIMER_A);
		return;
	}
    TimerDisable(TIMER0_BASE, TIMER_A);
}

//Configures interrupt pin supported by ADS7843 controller
static void touchIntPinInit()
{
	ADS7843_PORT_INT_CLOCK();
	ADS7843_INT_INPUT();
	ADS7843_INT_CONFIG_AS_FALLING();
    IntMasterEnable();
    touchIntPinInterruptEnable(true);
}

static void touchIntPinInterruptEnable(bool enable)
{
	if(enable)
	{
		ADS7843_INT_INTERRUPT_ENABLE();
		return;
	}
	ADS7843_INT_INTERRUPT_DISABLE();
}


//init method for touch controller
void touchScreenInit()
{
	ADS7843init();
	touchScreenTimerInit();
	touchIntPinInit();
}


void touchScreenSetTouchCallback(int32_t (*callback)(uint32_t message, int32_t x, int32_t y))
{
	touchCallback = callback;

}

void TouchScreenIntHandler(void)
{
	GPIOPinIntClear(ADS7843_PORT_INT, ADS7843_PIN_INT);
	if(GPIOPinIntStatus(ADS7843_PORT_INT, false) & ADS7843_PIN_INT)
	{
		ADS7843touchPenIntHandler();
		if(ADS7843dataAvailable())
		{
			//disable further interrupts on this pin
			touchIntPinInterruptEnable(false);
		    // Enable Timer
		    touchScreenTimerEnable(true);

		    debugConsolePrintf("PEN_DOWN\r\n");
		}

	}
}

void TouchScreenTimer0AIntHandler(void)
{
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	ADS7843touchPenIntHandler();
	if(ADS7843dataAvailable())
	{
		if((++m_counter % TIMER_SCAN_PERIOD_MS) == 0) //after every 3ms
		{
			debugConsolePrintf("Number of interrupts: %d\r", m_counter);
		}
	}
	else
	{
	    // disable Timer
	    touchScreenTimerEnable(false);
		//enable it back
		touchIntPinInterruptEnable(true);
	}
}
