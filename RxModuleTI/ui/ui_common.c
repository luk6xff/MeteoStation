/*
 * ui_common.c
 *
 *  Created on: 25 mar 2017
 *      Author: igbt6
 */

#include "ui_common.h"
#include "ui_message_box.h"
#include "ui_keyboard.h"

#include "ui_screenMain.h"
#include "ui_screenSettings.h"


#define UI_TIMER_BASE TIMER2_BASE
#define UI_TIMER_TYPE TIMER_A
#define UI_TIMER_SYSCTL_TIMER_TYPE SYSCTL_PERIPH_TIMER2
#define UI_TIMER_CFG TIMER_CFG_32_BIT_PER_UP
#define UI_TIMER_INT_FLAGS (TIMER_TIMA_TIMEOUT)
#define UI_TIMER_INT_MODE (INT_TIMER2A)

#define UI_TIMER_CB_NUM 3

static tContext m_drawingCtx;

static volatile uint32_t m_msCounter = 0;

typedef struct
{
	uint16_t periodTime;
	void (*timerCb)(void);
} TimerCallback;

static uint8_t m_timerCbNum = 0;

static TimerCallback* m_timerCb[UI_TIMER_CB_NUM];

static ScreenContainer m_screens[SCREEN_NUM_OF_SCREENS] =
{
    {
        (tWidget *)&ui_screenMainBackground,
        SCREEN_MAIN, SCREEN_CONN_SETTINGS, SCREEN_MAIN, SCREEN_MAIN
    },
    {
        (tWidget *)&ui_screenSettingsBackground,
        SCREEN_MAIN, SCREEN_CONN_SETTINGS, SCREEN_CONN_SETTINGS, SCREEN_CONN_SETTINGS
    },
    {
        (tWidget *)&ui_screenWifiSetupBackground,
		SCREEN_CONN_SETTINGS, SCREEN_CONN_SETTINGS, SCREEN_CONN_SETTINGS, SCREEN_CONN_SETTINGS
    },
    {
        (tWidget *)&ui_screenSensorSetupBackground,
		SCREEN_CONN_SETTINGS, SCREEN_CONN_SETTINGS, SCREEN_CONN_SETTINGS, SCREEN_CONN_SETTINGS
    }
};

static Screens m_currentScreen = SCREEN_MAIN;


//
//@brief Forward declarations of static methods
//
static void uiTimerInit();
static void uiFrameDraw(const char* app_name);
static void uiDrawInitInfo();



//
//@brief Methods definitions
//
void uiSetCurrentScreen(Screens screen)
{
	m_currentScreen = screen;
}

Screens uiGetCurrentScreen()
{
	return m_currentScreen;
}

const ScreenContainer* uiGetCurrentScreenContainer()
{
	return  &m_screens[uiGetCurrentScreen()];
}


//*****************************************************************************
//
// @brief Update UI screens with settings, data and other values
//
//*****************************************************************************
void uiUpdateScreen()
{
	uiScreenMainUpdate();
	uiScreenSettingsUpdate();
}


//*****************************************************************************
// @brief Main init method which initializes all the UI components
//*****************************************************************************

void uiInit()
{
    GrContextInit(&m_drawingCtx, &g_ILI9320);
	uiTimerInit();
	uiMessageBoxInit(); //msgBox
	uiKeyboardInit(); //keyboard
    uiFrameDraw("Meteo Ubiad Stacja");
    WidgetAdd(WIDGET_ROOT, uiGetCurrentScreenContainer()->widget);
    WidgetPaint(WIDGET_ROOT);
    uiDrawInitInfo();
}

//*****************************************************************************
// Clears the main screens background.
//*****************************************************************************
void uiClearBackground()
{
    static const tRectangle sRect =
    {
        BG_MIN_X,
        BG_MIN_Y,
        BG_MAX_X,
        BG_MAX_Y,
    };
	GrContextForegroundSet(&m_drawingCtx, BG_COLOR_MAIN);
    GrRectFill(&m_drawingCtx, &sRect);
}

//*****************************************************************************
// @brief draws the application banner
//*****************************************************************************
static void uiFrameDraw(const char* app_name)
{
    tRectangle sRect;
    // Fill the top 24 rows of the screen with blue to create the banner.
    sRect.i16XMin = 0;
    sRect.i16YMin = 0;
    sRect.i16XMax = GrContextDpyWidthGet(&m_drawingCtx) - 1;
    sRect.i16YMax = 23;
    GrContextForegroundSet(&m_drawingCtx, ClrDarkBlue);
    GrRectFill(&m_drawingCtx, &sRect);

    // Put a Red box around the banner.
    GrContextForegroundSet(&m_drawingCtx, ClrRed);
    GrRectDraw(&m_drawingCtx, &sRect);

    // Put the application name in the middle of the banner.
    GrContextForegroundSet(&m_drawingCtx, ClrYellowGreen);
    GrContextFontSet(&m_drawingCtx, &g_sFontCm20);
    GrStringDrawCentered(&m_drawingCtx, app_name, -1,
                         GrContextDpyWidthGet(&m_drawingCtx) / 2, 8, 0);
}

//*****************************************************************************
// @brief draws the app startup info
//*****************************************************************************
static void uiDrawInitInfo()
{
    // Put the application name in the middle of the banner.
    GrContextForegroundSet(&m_drawingCtx, ClrRed);
    GrContextFontSet(&m_drawingCtx, &g_sFontCmss24);
    GrStringDrawCentered(&m_drawingCtx, "App Starting ...", -1,
                         GrContextDpyWidthGet(&m_drawingCtx)/2, BG_MAX_Y/2, 0);
}

tContext* uiGetMainDrawingContext()
{
	return &m_drawingCtx;
}

bool uiRegisterTimerCb(void(*cb)(void), uint16_t period)
{
	if(m_timerCbNum >= UI_TIMER_CB_NUM)
	{
		return false;
	}
	TimerCallback* newCb = (TimerCallback*)malloc(sizeof(TimerCallback));
	if(newCb != NULL)
	{
		newCb->timerCb = cb;
		newCb->periodTime = period;
		m_timerCb[m_timerCbNum++] = newCb;
		return true;
	}
	return false;
}

bool uiUnRegisterTimerCb(void(*cb)(void))
{
	uint8_t i = 0;
	for(i = 0; i < m_timerCbNum; i++)
	{
		if(m_timerCb[i]->timerCb == cb)
		{
			free(m_timerCb[i]);
			m_timerCbNum--;
			return true;
		}
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////
//HW Dependent stuff below
//*****************************************************************************
//@brief Fires up/down the timer
//*****************************************************************************
static void uiTimerEnable(bool enable)
{
	if(enable)
	{
		TimerEnable(UI_TIMER_BASE, UI_TIMER_TYPE);
		return;
	}
    TimerDisable(UI_TIMER_BASE, UI_TIMER_TYPE);
}

//*****************************************************************************
//@brief Configures Timer2A as a 32-bit periodic timer
//*****************************************************************************
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

//*****************************************************************************
//@brief Timer2A interrupt handler
//*****************************************************************************
void UiTimer2AIntHandler(void)
{
	TimerIntClear(TIMER2_BASE, TIMER_TIMA_TIMEOUT);
	++m_msCounter;
	for(uint8_t i = 0; i < m_timerCbNum; i++)
	{
		if(m_timerCb[i] != NULL && m_timerCb[i]->timerCb != NULL)
		{
			if((m_msCounter % m_timerCb[i]->periodTime) == 0)
			{
				(*m_timerCb[i]->timerCb)();
			}
		}
	}
}

//*****************************************************************************
//@brief Delay for all UI methods
//*****************************************************************************
void uiDelay(uint32_t msDelay)
{
	uint32_t start = m_msCounter;
	while((m_msCounter - start) < msDelay)
	{
	}
}

//*****************************************************************************
//@brief Returns current ms Timer2A counter value
//*****************************************************************************
uint32_t uiDelayCounterMsVal()
{
	return m_msCounter;
}
