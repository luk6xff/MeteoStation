/*
 * ui_common.c
 *
 *  Created on: 25 mar 2017
 *      Author: igbt6
 */

#include "ui_common.h"
#include "ui_message_box.h"

#define UI_TIMER_BASE TIMER2_BASE
#define UI_TIMER_TYPE TIMER_A
#define UI_TIMER_SYSCTL_TIMER_TYPE SYSCTL_PERIPH_TIMER2
#define UI_TIMER_CFG TIMER_CFG_32_BIT_PER_UP
#define UI_TIMER_INT_FLAGS (TIMER_TIMA_TIMEOUT)
#define UI_TIMER_INT_MODE (INT_TIMER2A)


static tContext* m_drawingCtx = NULL;

static volatile uint32_t m_msCounter = 0;


//@brief Fires up/down the timer
static void uiTimerEnable(bool enable)
{
	if(enable)
	{
		TimerEnable(UI_TIMER_BASE, UI_TIMER_TYPE);
		return;
	}
    TimerDisable(UI_TIMER_BASE, UI_TIMER_TYPE);
}

//@brief Configures Timer2A as a 32-bit periodic timer
static void uiTimerInit()
{
    SysCtlPeripheralEnable(UI_TIMER_SYSCTL_TIMER_TYPE);
    TimerConfigure(UI_TIMER_BASE, UI_TIMER_CFG);
    // Set the Timer2A load value to 1ms.
    TimerLoadSet(UI_TIMER_BASE,  UI_TIMER_TYPE, SysCtlClockGet() / 1000);//1[ms]
    // Configure the Timer2A interrupt for timer timeout.
    TimerIntEnable(UI_TIMER_BASE, UI_TIMER_INT_FLAGS);
    // Enable the Timer2A interrupt on the processor (NVIC).
    IntEnable(UI_TIMER_INT_MODE);
    m_msCounter = 0;
    // Enable Timer2A.
    uiTimerEnable(true);
    //enable all interrupts in case of they are disabled
    ENABLE_ALL_INTERRUPTS();
}

//@brief Timer2A interrupt handler
void UiTimer2AIntHandler(void)
{
	TimerIntClear(TIMER2_BASE, TIMER_TIMA_TIMEOUT);
	++m_msCounter;
}

//@brief Delay for all UI methods
void uiDelay(uint32_t msDelay)
{
	uint32_t start = m_msCounter;
	while((m_msCounter - start) < msDelay)
	{
	}
}

//@brief Returns current ms Timer2A counter value
uint32_t uiDelayCounterMsVal()
{
	return m_msCounter;
}


/*@brief Main init method which initializes all the UI components*/
void uiInit(tContext* mainDrawingContext)
{
	if(mainDrawingContext == NULL)
	{
		abort(); //crash the APP if NULL
	}
	m_drawingCtx = mainDrawingContext;
	uiTimerInit();
	uiMessageBoxInit(); //msgBox
}

tContext* uiGetMainDrawingContext()
{
	return m_drawingCtx;
}


