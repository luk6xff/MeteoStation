/*
 * touch.c
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

#include "grlib/grlib.h"
#include "grlib/widget.h"

#include "system.h"

#include "touch.h"
#include "ads7843.h"

#include "debugConsole.h"  //FOR DEBUG ONLY

#define TOUCH_DEBUG_ENABLE 0
#define TOUCH_DEBUG(str) \
		do \
		{  \
			if (TOUCH_DEBUG_ENABLE) \
				debugConsolePrintf(str); \
		}while(0);

#define TIMER_SCAN_PERIOD_MS 5 //
//touch callback
static int32_t (*touchCallback)(uint32_t message, int32_t x, int32_t y);
static volatile uint32_t m_counter = 0; //increments every 1 [ms]
static volatile uint8_t m_WidgetPtrStatus = 0; //status of touch pointer passed to touch callback: WIDGET_MSG_PTR_DOWN, WIDGET_MSG_PTR_MOVE, WIDGET_MSG_PTR_UP
static volatile uint8_t m_MoveTouchStatus = 0;


//private methods declarations
static void touchScreenTimerInit();
static void touchScreenTimerEnable(bool enable);
static void touchIntPinInit();
static void touchIntPinInterruptEnable(bool enable);

void TouchScreenIntHandler(void);

//Configures Timer0A as a 32-bit periodic timer
static void touchScreenTimerInit()
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

    TimerConfigure(TIMER0_BASE, TIMER_CFG_32_BIT_PER_UP);
    // Set the Timer0A load value to 1ms.
    TimerLoadSet(TIMER0_BASE, TIMER_A, SysCtlClockGet() / 1000); //1 [ms]

    //enable all interrupts
    ENABLE_ALL_INTERRUPTS();

    // Configure the Timer0A interrupt for timer timeout.
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    // Enable the Timer0A interrupt on the processor (NVIC).
    IntEnable(INT_TIMER0A);

    m_counter = 0;

    // Disable Timer0A.
    touchScreenTimerEnable(true);
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
	GPIOPortIntRegister(ADS7843_PORT_INT, TouchScreenIntHandler);
	ENABLE_ALL_INTERRUPTS();
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


//Init method for touch controller
void touchScreenInit()
{
	ADS7843init();
	touchScreenTimerInit();
	//touchIntPinInit(); //not used
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
		touchIntPinInterruptEnable(false);
		ADS7843touchPenIntHandler();
		if(ADS7843dataAvailable())
		{
		    // Enable Timer
		    touchScreenTimerEnable(true);
		    TOUCH_DEBUG("PEN_DOWN\r\n");
		}
		else
		{
			touchIntPinInterruptEnable(true);
		}

	}
}

void TouchScreenTimer0AIntHandler(void)
{
	static uint8_t lastWidgetPtrStatus = 0;
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	if((++m_counter % TIMER_SCAN_PERIOD_MS) == 0) //after every TIMER_SCAN_PERIOD_MS [ms]
	{
		ADS7843touchPenIntHandler();
		if(ADS7843dataAvailable())
		{
			m_WidgetPtrStatus = WIDGET_MSG_PTR_DOWN;
			//TOUCH_DEBUG("Number of interrupts: %d\r", m_counter);
			TOUCH_DEBUG("PEN_DOWN\r\n");
			if(++m_MoveTouchStatus > (TIMER_SCAN_PERIOD_MS*20))
			{
				m_WidgetPtrStatus = WIDGET_MSG_PTR_MOVE;
				TOUCH_DEBUG("PEN_MOVE\r\n");
			}
		}
		else
		{
			m_MoveTouchStatus = 0;
			m_WidgetPtrStatus = WIDGET_MSG_PTR_UP;
			TOUCH_DEBUG("PEN_UP\r\n");
		}
		if(touchCallback && ((m_WidgetPtrStatus == WIDGET_MSG_PTR_MOVE) || (lastWidgetPtrStatus != m_WidgetPtrStatus)))
		{
			ADS7843read();
			touchCallback(m_WidgetPtrStatus, ADS7843getTouchedPoint().x, ADS7843getTouchedPoint().y);
		}
		lastWidgetPtrStatus = m_WidgetPtrStatus;
	}
}
