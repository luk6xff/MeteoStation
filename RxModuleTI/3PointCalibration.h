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

void drawCalibrationPoint(const tContext* ctx, uint16_t x, uint16_t y, uint16_t radius);


#endif /* _3POINTCALIBRATION_H_ */
