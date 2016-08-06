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
#include "SPFD5408.h"
#include "bitmaps.h"
#include "utils.h"

/**************************************************************************//**
 * @brief  Main function
 *****************************************************************************/
int main(void) {
	/* Chip errata */
	CHIP_Init();
	CMU_OscillatorEnable(cmuOsc_HFRCO, true, true);
	CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFRCO); //32MHZ
	CMU_ClockEnable(cmuClock_HFPER, true);
	//----------------------- SPFD5408 first working tests -----------------------
	//BSP_TraceProfilerSetup();
	/*BSP_LedsInit();
	 BSP_LedSet(0);
	 BSP_LedSet(1);*/
	utilsInit();
	SPFD5408init();

	uint16_t i = 0;
	uint16_t counter = 0;
	char buf[25] = { 0 };

	SPFD5408printChar('H', 10, 10, BLACK);
	SPFD5408printChar('E', 25, 10, BLACK);
	SPFD5408printChar('L', 40, 10, BLACK);
	SPFD5408printChar('L', 55, 10, BLACK);
	SPFD5408printChar('O', 70, 10, BLACK);
	SPFD5408printChar('!', 85, 10, BLACK);
	SPFD5408print("*TFTLibrary- TEST*", 10, 50, 0, RED);
	Delay(2000);
	for (int i = 1; i < 318; i++) {
		SPFD5408drawPixel(i, 119 + (sin(((i * 1.13) * 3.14) / 180) * 95),
				BLACK);
		Delay(1);

	}
	while (1) {

	}

}

