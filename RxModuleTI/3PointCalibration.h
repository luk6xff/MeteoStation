/*
 * 3PointCalibration.h
 *
 *  Created on: 18 lis 2016
 *      Author: igbt6
 */

#ifndef _3POINTCALIBRATION_H_
#define _3POINTCALIBRATION_H_
#include "grlib/grlib.h"
#include "ads7843.h"

//*****************************************************************************
//@brief  Touch screen calibration.
//*****************************************************************************
uint8_t performThreePointCalibration(tContext* ctx, CalibCoefficients* coefs);

uint8_t confirmThreePointCalibration(tContext* ctx);


#endif /* _3POINTCALIBRATION_H_ */
