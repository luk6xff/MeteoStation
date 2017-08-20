/*
 * ui_common.h
 *
 *  Created on: 25 mar 2017
 *      Author: igbt6
 */

#ifndef UI_UI_COMMON_H_
#define UI_UI_COMMON_H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"

#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/timer.h"
#include "driverlib/uart.h"

#include "../grlib/grlib.h"
#include "../grlib/widget.h"
#include "../grlib/canvas.h"
#include "../grlib/pushbutton.h"
#include "../grlib/checkbox.h"
#include "../grlib/container.h"
#include "../grlib/radiobutton.h"
#include "../grlib/keyboard.h"

#include "images.h"
#include "../system.h"
#include "../ILI9320_driver.h"

//*****************************************************************************
//
// Forward reference to all used widget structures.
//
//*****************************************************************************
extern tCanvasWidget ui_screenMainBackground;
extern tCanvasWidget ui_screenWifiSetupBackground;
extern tCanvasWidget ui_screenSensorSetupBackground;
extern tCanvasWidget ui_screenSettingsBackground;

//*****************************************************************************
// Typedefs
//*****************************************************************************
typedef enum
{
	SCREEN_MAIN,
	SCREEN_CONN_SETTINGS,
	SCREEN_WIFI_SETTINGS,
	SCREEN_SENSOR_SETTINGS,
	SCREEN_KEYBOARD,
	SCREEN_NUM_OF_SCREENS
} Screens;

typedef struct
{
	tWidget *widget;
	Screens up;
	Screens down;
	Screens left;
	Screens right;
} ScreenContainer;

#define KEYBOARD_MAX_TEXT_LEN 25 //25 chars to be typed in the keyboard
//*****************************************************************************
// Methods
//*****************************************************************************
void uiInit();
void uiSetCurrentScreen(Screens screen);
void uiClearBackground();
Screens uiGetCurrentScreen();
const ScreenContainer* uiGetCurrentScreenContainer();
tContext* uiGetMainDrawingContext();
void uiUpdateScreen();
void uiDelay(uint32_t msDelay);
uint32_t uiDelayCounterMsVal();
bool uiRegisterTimerCb(void(*cb)(void), uint16_t period);
bool uiUnRegisterTimerCb(void(*cb)(void));


#endif /* UI_UI_COMMON_H_ */
