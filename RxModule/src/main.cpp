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
#include "../emdrv/uartdrv/uartdrv.h"


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
	ADS7843Init();
	//UARTDRV_InitUart(handle, &initData);

	// Transmit data using a non-blocking transmit function
	//UARTDRV_Transmit(handle, buffer, 10, NULL);
	ili9320.ILI9320print("*TFTLibrary- TEST*", 10, 50, 0, ILI9320::Colors::RED);
	while (1) {
		if (getTouchStatus()==TOUCH_STATUS_TOUCHING)
		{
			uint16_t x, y;
			ADS7843ReadPointXY(&x, &y);
			getTouchPointCoordinates(&x, &y);
			ili9320.ILI9320drawPixel(x, y, ILI9320::Colors::BLACK);
			ili9320.ILI9320drawPixel(x+1, y+1, ILI9320::Colors::BLACK);
			ili9320.ILI9320drawPixel(x-1, y-1, ILI9320::Colors::BLACK);
			ili9320.ILI9320drawPixel(x, y+1, ILI9320::Colors::BLACK);
			ili9320.ILI9320drawPixel(x, y-1, ILI9320::Colors::BLACK);
			ili9320.ILI9320drawPixel(x+1, y, ILI9320::Colors::BLACK);
			ili9320.ILI9320drawPixel(x-1, y, ILI9320::Colors::BLACK);

			//mADS7843ScreenTouched = false;
			//ADS7843_INT_IRQ_CONFIG_FALLING(true);
		}
	}
}

