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
#include "../grlib/slider.h"
#include "../grlib/keyboard.h"

#include "../system.h"
#include "../ILI9320_driver.h"

//*****************************************************************************
// Define NULL, if not already defined.
//*****************************************************************************
#ifndef NULL
#define NULL	((void *)0)
#endif


void uiInit(tContext* mainDrawingContext);
tContext* uiGetMainDrawingContext();
void uiDelay(uint32_t msDelay);
uint32_t uiDelayCounterMsVal();

#endif /* UI_UI_COMMON_H_ */
