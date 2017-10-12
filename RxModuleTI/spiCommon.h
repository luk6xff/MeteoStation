/*
 * SpiCommon.h
 *
 *  Created on: 12 paü 2017
 *      Author: igbt6
 */

#ifndef SPICOMMON_H_
#define SPICOMMON_H_
#include "system.h"

void spiCommonInit(void);

bool spiCommonIsSpiInitialized(void);

bool spiCommonIsSpiBusy(void);

#endif /* SPICOMMON_H_ */
