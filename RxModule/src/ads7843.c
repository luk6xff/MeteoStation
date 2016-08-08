/*
 * ads7843.c
 *
 *  Created on: 08-08-2016
 *      Author: igbt6
 *      based on coocox library
 */
#include "spi.h"
#include "ADS7843.h"
#include "stdio.h"
#include "../emdrv/ustimer/ustimer.h"
#include "../emdrv/gpiointerrupt/gpiointerrupt.h"
#include "bsp.h"
#include "em_cmu.h"

//privates
/* USART used for SPI access */
#define USART_USED        USART1
#define USART_CLK         cmuClock_USART1
#define SPI_SW_ENABLED 0



static const USART_InitSync_TypeDef initSpi = { usartEnable, /* Enable RX/TX when init completed. */
1000000, /* Use 1MHz reference clock */
1000, /* 1 Mbits/s. */
usartDatabits8, /* 8 databits. */
true, /* Master mode. */
true, /* Send most significant bit first. */
usartClockMode0, false, usartPrsRxCh0, false };

static StatusTypeDef spiPeripheralsConfig(void) {

	CMU_ClockEnable(cmuClock_HFPER, true);
	// Enable clock for USART1
	CMU_ClockEnable(USART_CLK, true);
	CMU_ClockEnable(cmuClock_GPIO, true);

	// Reset USART just in case
	USART_Reset(USART_USED );

	USART_InitSync(USART_USED, &initSpi);

	// Module USART1 is configured to location 1
	USART_USED ->ROUTE = (USART_USED ->ROUTE & ~_USART_ROUTE_LOCATION_MASK)
			| USART_ROUTE_LOCATION_LOC1;

	// Enable signals TX, RX, CLK, CS
	USART_USED ->ROUTE |= USART_ROUTE_TXPEN | USART_ROUTE_RXPEN
			| USART_ROUTE_CLKPEN;

	//USART_USED ->CTRL |= USART_CTRL_CLKPOL_IDLEHIGH;
	//USART_USED ->CTRL |= USART_CTRL_CLKPHA_SAMPLELEADING;
	USART_USED ->CMD = USART_CMD_TXEN | USART_CMD_RXEN;
	// Clear previous interrupts
	USART_USED ->IFC = _USART_IFC_MASK;
#ifdef ADS7843_USE_PIN_BUSY
	ADS7843_BUSY_INPUT();


#endif
	ADS7843_MOSI_OUTPUT();
	ADS7843_MISO_INPUT();
	ADS7843_CLK_OUTPUT();
	ADS7843_CS_OUTPUT();
	//GPIO_PinModeSet(ADS7843_PORT_MOSI, ADS7843_PIN_MOSI, gpioModePushPull, 1);
	//GPIO_PinModeSet(ADS7843_PORT_MISO, ADS7843_PIN_MISO, gpioModeInput, 0);
	//GPIO_PinModeSet(ADS7843_PORT_CLK, ADS7843_PIN_CLK, gpioModePushPull, 0);
	// Keep CS high to not activate slave
	//GPIO_PinModeSet(ADS7843_PORT_CS, ADS7843_PIN_CS, gpioModePushPull, 1);
	return STATUS_OK;
}

void ADS7843spiInitSoftware(void) {
	CMU_ClockEnable(cmuClock_GPIO, true);
	ADS7843_MOSI_OUTPUT();
	ADS7843_MISO_INPUT();
	ADS7843_CLK_OUTPUT();
	// Keep CS high to not activate slave
	ADS7843_CS_OUTPUT();
	ADS7843_CLK_LOW();
	ADS7843_MOSI_LOW();
}


TouchInfo tTouchData;
static TouchPoint mPointCoordinates = { 100 };
extern volatile bool mADS7843ScreenTouched;
#define ADS7843_ENABLE_TOUCH_INT

static SpiHandleTypeDef spiHandle;
static void ADS7843PenIRQCallback(uint8_t pin);
//*****************************************************************************
// \brief Initialize ADS7843
// \param None.
// This function initialize ADS7843's SPI interface and .
//return None.
//*****************************************************************************
void ADS7843Init(void) {

#if SPI_SW_ENABLED

	ADS7843spiInitSoftware();
#ifdef ADS7843_USE_PIN_BUSY
	ADS7843_BUSY_INPUT();
	//ADS7843_BUSY_PULLED_INPUT();
#endif

#else
	spiHandle.spiInstance = USART_USED;
	spiHandle.init = &initSpi;
	spiHandle.spiModeHwSw = false;
	spiHandle.spiGpioClockInit = spiPeripheralsConfig;
	spiInit(&spiHandle);
#endif
#ifdef ADS7843_ENABLE_TOUCH_INT

	// Initialize GPIO interrupt dispatcher
	GPIOINT_Init();
	ADS7843_INT_INPUT();

	GPIOINT_CallbackRegister(ADS7843_PIN_INT, ADS7843PenIRQCallback);
	ADS7843_INT_IRQ_CONFIG_FALLING(true); //falling edge

#endif

	// assign default values
	tTouchData.thAdLeft = TOUCH_AD_X_MIN;
	tTouchData.thAdRight = TOUCH_AD_X_MAX;
	tTouchData.thAdUp = TOUCH_AD_Y_MAX;
	tTouchData.thAdDown = TOUCH_AD_Y_MIN;
	tTouchData.touchStatus = 0;
}

static uint16_t ADS7843SpiReadData(uint8_t reg) {

	uint8_t txBuf[3] = { reg, 0, 0 };
	uint8_t rxBuf[3] = { 0 };

	if (spiTransmitReceive(&spiHandle, txBuf, rxBuf, 3, 0) == STATUS_OK) {

		uint16_t retVal = ((rxBuf[1] << 4) | (rxBuf[2] >> 4)) & 0x0FFF;
		return retVal;
	} else
		return 0xffff;
}

static void ADS7843SpiWriteByteSoftware(uint8_t data) {
	///////////////////////////////////////////////////////////////	Delay(1);

	for (int i = 0; i < 100; i++)
		;

	for (int i = 0; i < 8; i++) {

		if (data & 0x80) {
			ADS7843_MOSI_HIGH();
		} else {
			ADS7843_MOSI_LOW();
		}
		ADS7843_CLK_HIGH();
		for (int i = 0; i < 100; i++)
			;
		///////////////////////////////////////////////////////////////		Delay(1);
		ADS7843_CLK_LOW();
		data <<= 1;
		for (int i = 0; i < 100; i++)
			;
		///////////////////////////////////////////////////////////////		Delay(1);

	}
	//ADS7843_MOSI_LOW();
}

static uint8_t ADS7843SpiReadByteSoftware(void) {
	uint8_t data = 0;

	for (int i = 0; i < 100; i++)
		;

	for (int i = 0; i < 8; i++) {
		ADS7843_CLK_HIGH();

		for (int i = 0; i < 100; i++)
			;

		data |=
				((GPIO_PinInGet(ADS7843_PORT_MISO, ADS7843_PIN_MISO) << (7 - i)));
		ADS7843_CLK_LOW();

		for (int i = 0; i < 100; i++)
			;
	}

	//ADS7843_MOSI_LOW();
	for (int i = 0; i < 100; i++)
		;
	return data;

}

static void ADS7843PenIRQCallback(uint8_t pin) {

	if (pin == ADS7843_PIN_INT && !mADS7843ScreenTouched) {
		uint16_t x, y;
		ADS7843_INT_IRQ_CONFIG_PIN_DISABLE();

		if (ADS7843_GET_INT_PIN()) {
			// Change to falling trigger edge when pen up
			tTouchData.touchStatus |= TOUCH_STATUS_PENUP;
			tTouchData.touchStatus &= ~TOUCH_STATUS_PENDOWN;
			ADS7843_INT_IRQ_CONFIG_FALLING(true);
		} else {

			// Modify status
			tTouchData.touchStatus |= TOUCH_STATUS_PENDOWN;
			tTouchData.touchStatus &= ~TOUCH_STATUS_PENUP;
			// Read x,y coordinate
			ADS7843ReadPointXY(&x, &y);
			mPointCoordinates.x = x;
			mPointCoordinates.y = y;
			mADS7843ScreenTouched = true;
			ADS7843_INT_IRQ_CONFIG_FALLING(true);
			//ADS7843_INT_IRQ_CONFIG_RISING(true);
		}


/*
		ADS7843ReadPointXY(&x, &y);
		mPointCoordinates.x = x;
		mPointCoordinates.y = y;
		mADS7843ScreenTouched = true;
		ADS7843_INT_IRQ_CONFIG_FALLING(true);
		*/
	}

}

uint16_t ADS7843PenInq(void) {
	return (uint16_t) ADS7843_GET_INT_PIN();
}

//****************************************************************************
// \brief Read the x, y axis ADC convert value once from ADS7843
// \param x To save the x axis ADC convert value.
// \param y To save the y axis ADC convert value.
// \return None.
//*****************************************************************************
#if SPI_SW_ENABLED
void ADS7843ReadADXYRaw(uint16_t *x, uint16_t *y) {
// Chip select

	ADS7843_CS_LOW();
	// Send read x command
	ADS7843SpiWriteByteSoftware(ADS7843_READ_X); //read x command
#ifdef ADS7843_USE_PIN_BUSY
	// wait for conversion complete
	while (ADS7843_GET_BUSY_PIN())
		;
#else
	for (volatile int i = 0; i < 1000; i++);
#endif

	// Read the high 8bit of the 12bit conversion result
	*x = (uint16_t) ADS7843SpiReadByteSoftware() & 0xFF;
	*x <<= 4;
	// Read the low 4bit of the 12bit conversion result
	*x |= (uint16_t) ADS7843SpiReadByteSoftware() >> 4;
	ADS7843_CS_HIGH();

	for (int ii = 0; ii < 1000; ii++)
		;

// Send read y command
	ADS7843_CS_LOW();
	//for (int x = 0; x < 0x100; x++);
	ADS7843SpiWriteByteSoftware(ADS7843_READ_Y); //read y command
#ifdef ADS7843_USE_PIN_BUSY
	// wait for conversion complete
	while (ADS7843_GET_BUSY_PIN())
		;
#else
	// The conversion needs 8us to complete
	for (volatile int i = 0; i < 1000; i++);
#endif

	// Read the high 8bit of the 12bit conversion result
	*y = (uint16_t) ADS7843SpiReadByteSoftware() & 0xFF;
	*y <<= 4;

	// Read the low 4bit of the 12bit conversion result
	*y |= (uint16_t) ADS7843SpiReadByteSoftware() >> 4;
	ADS7843_CS_HIGH();
	for (int ii = 0; ii < 1000; ii++)
		;

}
#else
// HW SPI
void ADS7843ReadADXYRaw(uint16_t *x, uint16_t *y) {
// Chip select
	for (int i = 0; i < 10000; i++)
	;
	ADS7843_CS_LOW();

	*x = ADS7843SpiReadData(ADS7843_READ_X);
#ifdef ADS7843_USE_PIN_BUSY
	// wait for conversion complete
	while(ADS7843_GET_BUSY_PIN());
#else
	for (volatile int i = 0; i < 1000; i++);
#endif
	*y = ADS7843SpiReadData(ADS7843_READ_Y);
	ADS7843_CS_HIGH();

}

#endif

//*****************************************************************************
//brief read the x, y axis ADC convert value from ADS7843(with software filter)
// \param x To save the x axis ADC convert value.
// \param y To save the y axis ADC convert value.
// This function Reads the x,y axis's ADC value from ADS7843 with software
// filter. The function first read out TOUCH_SMAPLE_LEN samples. Then discard
// the first data and the last data and sort the remained data. Then discard
// the first and the last one of the remained data and compute the average
// value of the final remained data.
// \return None.
//*****************************************************************************
void ADS7843ReadADXY(uint16_t *x, uint16_t *y) {
	uint16_t usXYArray[2][TOUCH_SMAPLE_LEN];
	uint8_t i, j;
	uint16_t temp;

	for (i = 0; i < TOUCH_SMAPLE_LEN; i++) {
		ADS7843ReadADXYRaw(&usXYArray[0][i], &usXYArray[1][i]);
	}
// Discard the first and the last one of the data and sort remained data
	for (i = 1; i < TOUCH_SMAPLE_LEN - 2; i++) {
		for (j = i + 1; j < TOUCH_SMAPLE_LEN - 1; j++) {
			if (usXYArray[0][i] > usXYArray[0][j]) {
				temp = usXYArray[0][i];
				usXYArray[0][i] = usXYArray[0][j];
				usXYArray[0][j] = temp;
			}

			if (usXYArray[1][i] > usXYArray[1][j]) {
				temp = usXYArray[1][i];
				usXYArray[1][i] = usXYArray[1][j];
				usXYArray[1][j] = temp;
			}
		}
	}
	usXYArray[0][0] = 0;
	usXYArray[1][0] = 0;

//
// Discard the first and the last one of the sorted data
// and compute the average value of the remained data.
//
	for (i = 2; i < TOUCH_SMAPLE_LEN - 2; i++) {
		usXYArray[0][0] += usXYArray[0][i];
		usXYArray[1][0] += usXYArray[1][i];
	}
	*x = usXYArray[0][0] / (TOUCH_SMAPLE_LEN - 4);
	*y = usXYArray[1][0] / (TOUCH_SMAPLE_LEN - 4);

}

//*****************************************************************************
// \brief Read the XY coordinate of touch point.
// \param x To save the x coordinate
// \param y To save the y coordinate
// This function is to read current touch point. The x,y coordinates will be
// read out from the input parameters. If the screen is not touched, it will
// return last value, after the last value is read out, additional read will
// return fail information and nothing will be read out.
// \return None.
//*****************************************************************************
uint8_t ADS7843ReadPointXY(uint16_t *x, uint16_t *y) {
	uint16_t usADX;
	uint16_t usADY;
	uint32_t temp = 0;

	if (!ADS7843_GET_INT_PIN()) {
		//  If pen down
		ADS7843ReadADXY(&usADX, &usADY);
		*x = usADX;
		*y = usADY;
		return 0;
		//  calculate the difference
		temp = usADX - tTouchData.thAdLeft;

		//  limit the left edge
		if (temp > tTouchData.thAdRight)
			temp = 0;

		//  calculate the x coordinate
		*x = temp * TOUCH_SCREEN_WIDTH
				/ (tTouchData.thAdRight - tTouchData.thAdLeft);
		tTouchData.lastX = tTouchData.curX;
		tTouchData.curX = *x;

		temp = tTouchData.thAdUp - usADY;

		//  limit the top edge
		if (temp > tTouchData.thAdUp)
			temp = 0;

		//  calculate the y coordinate
		*y = temp * TOUCH_SCREEN_HEIGHT
				/ (tTouchData.thAdUp - tTouchData.thAdDown);
		tTouchData.lastY = tTouchData.curY;
		tTouchData.curY = *y;
		return 0;
	} else {
		//  if pen up
		if (tTouchData.touchStatus & TOUCH_STATUS_PENUP) {
			tTouchData.lastX = tTouchData.curX;
			*x = tTouchData.curX;
			tTouchData.lastY = tTouchData.curY;
			*y = tTouchData.curY;
			tTouchData.touchStatus = 0;
			return 0;
		}

		return 1;
	}
}

TouchPoint getCoordinates(void) {

	return mPointCoordinates;
}

uint16_t ADS7843ReadInputChannel(unsigned char ucChannel) {
	uint16_t res;

	if ((ucChannel != ADS7843_READ_IN4) || (ucChannel != ADS7843_READ_IN3)
			|| (ucChannel != ADS7843_READ_X) || (ucChannel != ADS7843_READ_Y)) {
		return 0;
	}
	ADS7843_CS_LOW();
	Delay(10);
	ADS7843SpiWriteByteSoftware(ucChannel);
#ifdef ADS7843_USE_PIN_BUSY
	// wait for conversion complete
	while (ADS7843_GET_BUSY_PIN())
		;
#else
	for (volatile int i = 0; i < 1000; i++);
#endif
	res = (uint16_t) ADS7843SpiReadByteSoftware() & 0x00FF;
	res <<= 4;
	res |= (uint16_t) ADS7843SpiReadByteSoftware() >> 4;
	ADS7843_CS_HIGH();

	return res;
}

//*****************************************************************************
// \brief Touch screen calibration.
// \param None.
// This function is to calibrate the touch screen. If the read out coordinate
// is not accurate, you can use this function to calibrate it. After this
// function is called, you must touch the screen in about 10 seconds or the
// function will return with no effect. After you touch the screen, the counter
// will be zeroed and restart counting. If the calibration is not over and the
// touch is always applied on the screen, the counter will not increase, so you
// don't have to worry about the calibrate time.
//     All you need to do with the calibration is to use a touch pen or other
// object which is a little sharp but will not damage the touch screen to touch
// the screen and slide the pen from top edge of the screen to the bottom edge.
// Then slide the pen from left edge to right edge. Make sure that the pen should
// not leave the screen until it slides to the edge or the calibrate may be
// inaccurate. The direction or sequence is optional. If __DEBUG_PRINT__ is defined
// you can see the calibration result. The calibration will calibrate both x axis
// and y axis. Also you can calibration only one axis. the axis which is not
// calibrated will retain its original value.
// \return None.
//
//*****************************************************************************
uint8_t ADS7843Calibration(void) {
	uint16_t adx, ady;
	uint16_t adxmin = TOUCH_AD_X_MAX;
	uint16_t adxmax = TOUCH_AD_X_MIN;
	uint16_t adymin = TOUCH_AD_X_MAX;
	uint16_t adymax = TOUCH_AD_X_MIN;
	uint16_t calibrationFlag = 0;
	uint32_t timeout = 0;

#ifdef ADS7843_ENABLE_TOUCH_INT
//  disable touch interrupt
	ADS7843_INT_IRQ_CONFIG_PIN_DISABLE();
#endif
	while (1) {
		if (!ADS7843_GET_INT_PIN()) {
			//
			// If pen down continuously read the x,y value and record the
			// maximum and minimum value
			//
			while (!ADS7843_GET_INT_PIN()) {
				ADS7843ReadADXY(&adx, &ady);
				if (adx < adxmin) {
					adxmin = adx;
				} else if (adx > adxmax) {
					adxmax = adx;
				}

				if (ady < adymin) {
					adymin = ady;
				} else if (ady > adymax) {
					adymax = ady;
				}
			}
			// Counter clear.
			timeout = 0;
		}

		// If x axis calibration is not complete
		if (!(calibrationFlag & 1)) {
			//  If x axis calibrate over
			if ((adxmax > (TOUCH_AD_X_MAX - TOUCH_AD_CALIB_ERROR))
					&& (adxmax < (TOUCH_AD_X_MAX + TOUCH_AD_CALIB_ERROR))
					&& (adxmin > (TOUCH_AD_X_MIN - TOUCH_AD_CALIB_ERROR))
					&& (adxmin < (TOUCH_AD_X_MIN + TOUCH_AD_CALIB_ERROR))) {

				tTouchData.thAdLeft = adxmin;
				tTouchData.thAdRight = adxmax;
				calibrationFlag |= 1;
			}
		}

		// If y axis calibration is not complete
		if (!(calibrationFlag & 2)) {
			//  If y axis calibrate over
			if ((adymax > (TOUCH_AD_Y_MAX - TOUCH_AD_CALIB_ERROR))
					&& (adymax < (TOUCH_AD_Y_MAX + TOUCH_AD_CALIB_ERROR))
					&& (adymin > (TOUCH_AD_Y_MIN - TOUCH_AD_CALIB_ERROR))
					&& (adymin < (TOUCH_AD_Y_MIN + TOUCH_AD_CALIB_ERROR))) {
				tTouchData.thAdUp = adymax;
				tTouchData.thAdDown = adymin;
				calibrationFlag |= 2;
			}
		}

		// If two coordinates calibrate over or timer out
		if ((calibrationFlag == 3) || (timeout++ > 100000)) {
#ifdef ADS7843_ENABLE_TOUCH_INT
			ADS7843_INT_IRQ_CONFIG_FALLING(true);
#endif
			return calibrationFlag;
		}
		Delay(1000);
	}
}
