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
#include "ili9320.h"
#include "ads7843.h"
#include "bitmaps.h"
#include "../emdrv/ustimer/ustimer.h"
#include "../emdrv/uartdrv/uartdrv.h"

volatile bool mADS7843ScreenTouched = false;

/*Uart driver */
// Define receive/transmit operation queues
DEFINE_BUF_QUEUE(EMDRV_UARTDRV_MAX_CONCURRENT_RX_BUFS, rxBufferQueueI0);
DEFINE_BUF_QUEUE(EMDRV_UARTDRV_MAX_CONCURRENT_TX_BUFS, txBufferQueueI0);

// Configuration for USART0, location 1
#define MY_UART                                     \
{                                                   \
  UART0,                                           \
  115200,                                           \
  _USART_ROUTE_LOCATION_LOC1,                       \
  usartStopbits1,                                   \
  usartNoParity,                                    \
  usartOVS8,                                       \
  false,                                            \
  false,                             \
  false,                                        \
  false,                                                \
  false,                                        \
  false,                                                \
  (UARTDRV_Buffer_FifoQueue_t *)&rxBufferQueueI0,   \
  (UARTDRV_Buffer_FifoQueue_t *)&txBufferQueueI0    \
}

UARTDRV_HandleData_t handleData;
UARTDRV_Handle_t handle = &handleData;

uint8_t buffer[10] = { "HELLO!" };
UARTDRV_InitUart_t initData = MY_UART;

TouchInfo touchInfoData;
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

	ILI9320init();
	ADS7843Init();
	//UARTDRV_InitUart(handle, &initData);

	// Transmit data using a non-blocking transmit function
	//UARTDRV_Transmit(handle, buffer, 10, NULL);

	/*
	 ILI9320printChar('H', 10, 10, BLACK);
	 ILI9320printChar('E', 25, 10, BLACK);
	 ILI9320printChar('L', 40, 10, BLACK);
	 ILI9320printChar('L', 55, 10, BLACK);
	 ILI9320printChar('O', 70, 10, BLACK);
	 ILI9320printChar('!', 85, 10, BLACK);
	 Delay(2000);
	 for (int i = 1; i < 318; i++) {
	 ILI9320drawPixel(i, 119 + (sin(((i * 1.13) * 3.14) / 180) * 95),
	 BLACK);
	 }
	 */
	ILI9320print("*TFTLibrary- TEST*", 10, 50, 0, RED);
	while (1) {
		if (mADS7843ScreenTouched)
		{
			uint16_t x, y;
			ADS7843ReadPointXY(&x, &y);

			getCoordinates(&x, &y);
			ILI9320drawPixel(x, y, BLACK);
			ILI9320drawPixel(x+1, y+1, BLACK);
			ILI9320drawPixel(x-1, y-1, BLACK);
			ILI9320drawPixel(x, y+1, BLACK);
			ILI9320drawPixel(x, y-1, BLACK);
			ILI9320drawPixel(x+1, y, BLACK);
			ILI9320drawPixel(x-1, y, BLACK);

			mADS7843ScreenTouched = false;
			ADS7843_INT_IRQ_CONFIG_FALLING(true);
		}
	}
}

