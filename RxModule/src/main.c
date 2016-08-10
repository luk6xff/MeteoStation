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
#include "spfd5408.h"
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

uint8_t buffer[10]= {"HELLO!"};
UARTDRV_InitUart_t initData = MY_UART;



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
	  // Initialization of USTIMER driver
	USTIMER_Init();

	SPFD5408init();
	ADS7843Init();
	//UARTDRV_InitUart(handle, &initData);

	// Transmit data using a non-blocking transmit function
	//UARTDRV_Transmit(handle, buffer, 10, NULL);

	/*
	SPFD5408printChar('H', 10, 10, BLACK);
	SPFD5408printChar('E', 25, 10, BLACK);
	SPFD5408printChar('L', 40, 10, BLACK);
	SPFD5408printChar('L', 55, 10, BLACK);
	SPFD5408printChar('O', 70, 10, BLACK);
	SPFD5408printChar('!', 85, 10, BLACK);
		Delay(2000);
	for (int i = 1; i < 318; i++) {
		SPFD5408drawPixel(i, 119 + (sin(((i * 1.13) * 3.14) / 180) * 95),
				BLACK);
	}
	*/
	SPFD5408print("*TFTLibrary- TEST*", 10, 50, 0, RED);
	int i = 0;
	while (1) {
		if (mADS7843ScreenTouched) {

					SPFD5408drawPixel(getCoordinates().x / 100,
							getCoordinates().y / 100, RED);
					uint16_t x, y;
					ADS7843ReadADXYRaw(&x, &y);
					//ADS7843ReadPointXY(&x, &y);
					mADS7843ScreenTouched=false;
					ADS7843_INT_IRQ_CONFIG_FALLING(true);
					SPFD5408drawPixel(i++%320, i%320,BLACK);
				}

	}

}

