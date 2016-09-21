/***************************************************************************//**
 * @file gpiointerrupt.h
 * @brief GPIOINT API definition
 * @version 4.4.0
 *******************************************************************************
 * @section License
 * <b>(C) Copyright 2014 Silicon Labs, http://www.silabs.com</b>
 *******************************************************************************
 *
 * This file is licensed under the Silabs License Agreement. See the file
 * "Silabs_License_Agreement.txt" for details. Before using this software for
 * any purpose, you must agree to the terms of that agreement.
 *
 ******************************************************************************/

#ifndef __EMDRV_GPIOINTERRUPT_H
#define __EMDRV_GPIOINTERRUPT_H

#include "stdint.h"
#include "../Callback.h"

/***************************************************************************//**
 * @addtogroup emdrv
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup GPIOINT
 * @brief GPIOINT General Purpose Input/Output Interrupt dispatcher
 * @{
 ******************************************************************************/

/*******************************************************************************
 *******************************   TYPEDEFS   **********************************
 ******************************************************************************/

/**
 * @brief
 *  GPIO interrupt callback function pointer.
 * @details
 *   Parameters:
 *   @li pin - The pin index the callback function is invoked for.
 */
typedef Callback<void(uint8_t)> GPIOINT_IrqCallbackPtr;
/*******************************************************************************
 ******************************   PROTOTYPES   *********************************
 ******************************************************************************/
void GPIOINT_Init(void);

void GPIOINT_CallbackRegister(uint8_t pin, GPIOINT_IrqCallbackPtr* callbackPtr);

static __INLINE void GPIOINT_CallbackUnRegister(uint8_t pin);

/***************************************************************************//**
 * @brief
 *   Unregisters user callback for given pin number.
 *
 * @details
 *   Use this function to unregister a callback.
 *
 * @param[in] pin
 *   Pin number for the callback.
 *
 ******************************************************************************/
static __INLINE void GPIOINT_CallbackUnRegister(uint8_t pin)
{
  GPIOINT_CallbackRegister(pin,nullptr);
}


#endif /* __EMDRV_GPIOINTERRUPT_H */
