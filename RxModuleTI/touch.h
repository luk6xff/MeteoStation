/*
 * touch.h
 *
 *  Created on: 20 lis 2016
 *      Author: igbt6
 */

#ifndef TOUCH_H_
#define TOUCH_H_

#include "system.h"


void touchScreenInit();


//*****************************************************************************
//
//! Sets the callback function for touch screen events.
//!
//! @param callback - a pointer to the function to be called when touch
//! screen events occur.
//! This function sets the address of the function to be called when touch
//! screen events occur.  The events that are recognized are the screen being
//! touched ("pen down"), the touch position moving while the screen is
//! touched ("pen move"), and the screen no longer being touched (''pen up'').
//! @return None.
//
//*****************************************************************************
void touchScreenSetTouchCallback(int32_t (*callback)(uint32_t message, int32_t x, int32_t y));

#endif /* TOUCH_H_ */
