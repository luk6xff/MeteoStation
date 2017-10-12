/*
 * ads7843.c
 *
 *  Created on: 08-08-2016
 *      Author: igbt6
 *
 */

#include "ads7843.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/ssi.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"


//
// Enable touch interrupt
//
//#define ADS7843_ENABLE_TOUCH_INT
#define TOUCH_SCREEN_WIDTH         240
#define TOUCH_SCREEN_HEIGHT        320

#define TOUCH_MAX_NUM_OF_SAMPLES    20  //Note! must be >= 7
//Configure it correctly to your display
#define TOUCH_AD_X_MAX             1880
#define TOUCH_AD_X_MIN             150
#define TOUCH_AD_Y_MAX             2020
#define TOUCH_AD_Y_MIN             170
#define TOUCH_AD_CALIB_ERROR       30

//*****************************************************************************
//
// @addtogroup ADS7843_Config ADS7843 Driver Predefines
// @brief This part defines the slave address and register address of ADS7843.
//
//*****************************************************************************
#define ADS7843_READ_X             0xD0
#define ADS7843_READ_Y             0x90
#define ADS7843_SER            	   0x04
#define ADS7843_DFR            	   0x00
#define ADS7843_NO_POWERDOWN       0x03
#define ADS7843_POWERDOWN          0x00
#define ADS7843_12_BIT             0x00
#define ADS7843_8_BIT              0x08
#define ADS7843_READ_IN3           0xA0
#define ADS7843_READ_IN4           0xE0



//privates
static TouchInfo touchInfoData;
static TouchPoint currentTouchedPoint;
//corection coefficients
static CalibCoefficients calibCoefs;

static void spiInit(void)
{
	if (!spiCommonIsSpiInitialized())
	{
		while(1);
	}
	ADS7843_PORT_CS_CLOCK();
	ADS7843_CS_OUTPUT(); //SSI0FSS will be handled manually
}

static void spiWriteReadData(uint8_t* txBuf, uint8_t txSize, uint8_t* rxBuf,
		uint8_t rxSize) {
	ADS7843_CS_LOW();
	if (txBuf != (void*) 0) {
		for (int txIdx = 0; txIdx < txSize; ++txIdx) {
			SSIDataPut(SSI0_BASE, txBuf[txIdx]);
			while (SSIBusy(SSI0_BASE)) {
			}
		}
	}

	if (rxBuf != (void*) 0) {
		//read bytes of data
		for (int rxIdx = 0; rxIdx < rxSize; ++rxIdx) {
			SSIDataGet(SSI0_BASE, &rxBuf[rxIdx]);
		}
	}
	ADS7843_CS_HIGH();
}

static uint16_t spiReadData(uint8_t reg) {
	uint8_t txBuf[3] = {reg, 0, 0};
	uint32_t rxBuf[3] = {0, 0, 0};
	uint16_t cur = 0x0000;
	ADS7843_CS_LOW();
	for (int idx = 0; idx < 3; ++idx) {
		SSIDataPut(SSI0_BASE, txBuf[idx]);
		while (SSIBusy(SSI0_BASE)) {
		}
	}
	//read 3 bytes of data
	for (int idx = 0; idx < 3; ++idx) {
		SSIDataGet(SSI0_BASE, &rxBuf[idx]);
		rxBuf[idx] &= 0x00FF;
	}
	cur = ((rxBuf[1] << 4) | (rxBuf[2] >> 4)) & 0x0FFF;
	ADS7843_CS_HIGH();
	return cur;
}

void ADS7843init(void) {

	spiInit();

#ifdef ADS7843_USE_PIN_BUSY
	ADS7843_BUSY_INPUT();
#endif

//#ifdef ADS7843_ENABLE_TOUCH_INT
	// Initialize GPIO interrupt dispatcher
	ADS7843_PORT_INT_CLOCK();
	ADS7843_INT_INPUT();
//#endif
	// assign default values
	touchInfoData.touchStatus = TOUCH_STATUS_PENUP;
	ADS7843setIrqAndPowerDown();
}

bool ADS7843read(bool rawDataMode)
{
	return ADS7843readPointXY(&currentTouchedPoint, rawDataMode);
}

bool ADS7843dataAvailable()
{
	if (ADS7843getTouchStatus() == TOUCH_STATUS_PENDOWN) {
		return true;
	}
	return false;
}

TouchPoint ADS7843getTouchedPoint()
{
	return currentTouchedPoint;
}

void ADS7843setIrqAndPowerDown(void) {
	uint8_t buf[3] = { ADS7843_READ_X | ADS7843_DFR, 0, 0 };
	spiWriteReadData(buf, 3, 0, 0);
}

void ADS7843touchPenIntHandler()
{

	if (!ADS7843getIntPinState()) //if pendown
	{
		//ADS7843_INT_IRQ_CONFIG_PIN_DISABLE();
		touchInfoData.touchStatus = TOUCH_STATUS_PENDOWN;
	} else {
		touchInfoData.touchStatus = TOUCH_STATUS_PENUP;
	}
}

bool ADS7843getIntPinState(void) {
	return  ADS7843_GET_INT_PIN()>0;
}

void ADS7843readRawXY(uint16_t *x, uint16_t *y) {
	*x = spiReadData(ADS7843_READ_X | ADS7843_DFR);
#ifdef ADS7843_USE_PIN_BUSY
	// wait for conversion complete
	while(ADS7843_GET_BUSY_PIN());
#else
	//for (volatile int i = 0; i < 1000; i++);
#endif
	*y = spiReadData(ADS7843_READ_Y | ADS7843_DFR);
}


uint16_t ADS7843fastMedian(uint16_t *samples) {
	// do a fast median selection  - code stolen from https://github.com/andysworkshop/stm32plus library
#define PIX_SORT(a,b) { if ((a)>(b)) PIX_SWAP((a),(b)); }
#define PIX_SWAP(a,b) { uint16_t temp=(a);(a)=(b);(b)=temp; }
	PIX_SORT(samples[0], samples[5]);
	PIX_SORT(samples[0], samples[3]);
	PIX_SORT(samples[1], samples[6]);
	PIX_SORT(samples[2], samples[4]);
	PIX_SORT(samples[0], samples[1]);
	PIX_SORT(samples[3], samples[5]);
	PIX_SORT(samples[2], samples[6]);
	PIX_SORT(samples[2], samples[3]);
	PIX_SORT(samples[3], samples[6]);
	PIX_SORT(samples[4], samples[5]);
	PIX_SORT(samples[1], samples[4]);
	PIX_SORT(samples[1], samples[3]);
	PIX_SORT(samples[3], samples[4]);
	return samples[3];
}

TouchPoint ADS7843translateCoordinates(const TouchPoint* rawPoint)
{
	TouchPoint p;
	p.x = calibCoefs.a_x * rawPoint->x + calibCoefs.b_x * rawPoint->y + calibCoefs.d_x;
	p.y = calibCoefs.a_y * rawPoint->x + calibCoefs.b_y * rawPoint->y + calibCoefs.d_y;
	return p;
}

bool ADS7843readPointXY(TouchPoint* touchPoint, bool rawDataMode)
{
	bool isPenDown = touchInfoData.touchStatus == TOUCH_STATUS_PENDOWN;

	uint16_t xyDataBuf[2][7]; //7 samples
	TouchPoint p;
	for (uint8_t i = 0; i < 7; i++)
	{
		ADS7843readRawXY(&xyDataBuf[0][i], &xyDataBuf[1][i]);
	}

	p.x = ADS7843fastMedian(xyDataBuf[0]);
	p.y = ADS7843fastMedian(xyDataBuf[1]);

	if (!rawDataMode)
	{
		*touchPoint = ADS7843translateCoordinates(&p);
	}
	else
	{
		*touchPoint = p;
	}
	return isPenDown;
}

inline TouchStatus ADS7843getTouchStatus() {
	return touchInfoData.touchStatus;
}

void ADS7843setCalibrationCoefficients(const CalibCoefficients* coeffs)
{
	calibCoefs.a_x = coeffs->a_x;
	calibCoefs.a_y = coeffs->a_y;
	calibCoefs.b_x = coeffs->b_x;
	calibCoefs.b_y = coeffs->b_y;
	calibCoefs.d_x = coeffs->d_x;
	calibCoefs.d_y = coeffs->d_y;

}


bool ADS7843readOnePointRawCoordinates(TouchPoint* point) {
	TouchPoint tempP;
	const uint16_t samplesNum = 500;
	uint16_t idx = 0;
	uint32_t xSum = 0;
	uint32_t ySum = 0;

	while (ADS7843getIntPinState())
		; //wait till we pen down the panel
	for (;;) {
		if (!ADS7843getIntPinState()) {
			ADS7843readPointXY(&tempP, true);
			xSum += tempP.x;
			ySum += tempP.y;
			++idx;
			if (idx == samplesNum)
				break;
		}
	}
	point->x = xSum / samplesNum;
	point->y = ySum / samplesNum;
	return true;
}


