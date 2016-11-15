/*
 * ILI9320_driver.h
 *
 *  Created on: 15 lis 2016
 *      Author: igbt6
 */

#ifndef ILI9320_DRIVER_H_
#define ILI9320_DRIVER_H_
#include "grlib/grlib.h"


//*****************************************************************************
//
// Prototypes for the globals exported by this driver.
//
//*****************************************************************************
extern void ILI9320Init(void);
extern void ILI9320BacklightOn(void);
extern unsigned short ILI9320ControllerIdGet(void);
extern void ILI9320BacklightOff(void);
extern const tDisplay g_ILI9320;



#endif /* ILI9320_DRIVER_H_ */
