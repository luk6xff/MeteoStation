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

#include "../system.h"
#include "../ILI9320_driver.h"


void uiInit(tContext* mainDrawingContext);
void uiClearBackground();
void uiFrameDraw(tContext* drawing_ctx, const char* app_name);
void uiDrawInitInfo();
tContext* uiGetMainDrawingContext();
void uiDelay(uint32_t msDelay);
uint32_t uiDelayCounterMsVal();
bool uiRegisterTimerCb(void(*cb)(void), uint16_t period);
bool uiUnRegisterTimerCb(void(*cb)(void));


#endif /* UI_UI_COMMON_H_ */
