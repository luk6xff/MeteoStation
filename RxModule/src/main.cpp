/**************************************************************************//**
 * @file
 * @brief Meteo Station
 * @author igbt6
 * @version 0.01
 *
 ******************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "ILI9320.h"
#include "ads7843.h"
#include "bitmaps.h"
#include "../emdrv/ustimer/ustimer.h"


/**************************************************************************//**
 * @brief  Main function
 *****************************************************************************/
int main(void) {
	/* Chip errata */
	CHIP_Init();
	CMU_OscillatorEnable(cmuOsc_HFXO, true, false);
	CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFXO); //32MHZ
	CMU_ClockDivSet(cmuClock_HF, cmuClkDiv_1);
	 CMU_ClockDivSet(cmuClock_HFPER, cmuClkDiv_1);
	CMU_ClockEnable(cmuClock_HFPER, true);
	//----------------------- ILI9320 first working tests -----------------------
	//BSP_TraceProfilerSetup();
	/*BSP_LedsInit();
	 BSP_LedSet(0);
	 BSP_LedSet(1);*/
	// Initialization of USTIMER driver

	USTIMER_Init();
	ILI9320 ili9320;
	ili9320.ILI9320init();
	ADS7843 ads7843;
	//ili9320.ILI9320print("*TFTLibrary- TEST*", 10, 50, 0, ILI9320::Colors::RED);
	ads7843.calibration(ili9320);
	while (1) {
		if (ads7843.getTouchStatus()==ADS7843::TOUCH_STATUS_TOUCHING)
		{
			//my method
			uint16_t x, y;
#if 1
			ads7843.readPointXY(&x, &y);
			ads7843.getTouchPointCoordinates(&x, &y);
			ili9320.ILI9320fillCircle(x,y,3, ILI9320::Colors::BLUE);
#else
			ADS7843::TouchPoint p;
			ads7843.readXYMedian(p);
			ili9320.ILI9320fillCircle(p.x,p.y,3, ILI9320::Colors::BLUE);
#endif
		}
	}
}

