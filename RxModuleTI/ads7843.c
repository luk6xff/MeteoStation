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





//privates
static TouchInfo m_touchInfoData;
static TouchPoint m_currentTouchedPoint;
//corection coefficients
static CalibCoefficients calibCoefs;

// Configure the SoftSSI module.  The size of the FIFO buffers can be
// Module SSI0, pins are assigned as follows:
//      PA2 - SoftSSICLK
//      PA3 - SoftSSIFss
//      PA4 - SoftSSIRx
//      PA5 - SoftSSITx
static void spiInit(void) {

	uint32_t rxBuf[1];
	SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);

	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	//GPIOPinConfigure(GPIO_PA3_SSI0FSS);
	ADS7843_PORT_CS_CLOCK();
	ADS7843_CS_OUTPUT(); //SSI0FSS will be handled manually

	GPIOPinConfigure(GPIO_PA2_SSI0CLK);
	GPIOPinConfigure(GPIO_PA4_SSI0RX);
	GPIOPinConfigure(GPIO_PA5_SSI0TX);

	GPIOPinTypeSSI(GPIO_PORTA_BASE,
	GPIO_PIN_5 | GPIO_PIN_4 | /*GPIO_PIN_3 |*/ GPIO_PIN_2);

	// Configure and enable the SSI port for SPI master mode.  Use SSI0,
	// system clock supply, idle clock level low and active low clock in
	// freescale SPI mode, master mode, 1MHz SSI frequency, and 8-bit data.
	SSIConfigSetExpClk(SSI0_BASE, SysCtlClockGet(), SSI_FRF_MOTO_MODE_0,
	SSI_MODE_MASTER, 1000000, 8);

	// Enable the SSI0 module.
	SSIEnable(SSI0_BASE);

	//flush the receives fifos
	while (SSIDataGetNonBlocking(SSI0_BASE, &rxBuf[0])) {
	}
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
	m_touchInfoData.touchStatus = TOUCH_STATUS_PENUP;
	ADS7843setIrqAndPowerDown();
}

bool ADS7843read() {
	return ADS7843readPointXY(&m_currentTouchedPoint, false);
}

bool ADS7843dataAvailable() {
	if (ADS7843getTouchStatus() ==TOUCH_STATUS_TOUCHING) {
		return true;
	}
	return false;
}

TouchPoint ADS7843getTouchedPoint()
{
	return m_currentTouchedPoint;
}

void ADS7843setIrqAndPowerDown(void) {
	uint8_t buf[3] = { ADS7843_READ_X | ADS7843_DFR, 0, 0 };
	spiWriteReadData(buf, 3, 0, 0);
}

void ADS7843penIRQCallback(uint8_t pin) {
	/*
	 if (pin == ADS7843_PIN_INT) {
	 if (!ADS7843getIrqPinState()) //if pendown
	 {
	 //ADS7843_INT_IRQ_CONFIG_PIN_DISABLE();
	 ADS7843_INT_IRQ_CONFIG_FALLING(false);
	 ADS7843_INT_IRQ_CONFIG_RISING(true);
	 m_touchInfoData.touchStatus = TOUCH_STATUS_TOUCHING;
	 } else {
	 ADS7843_INT_IRQ_CONFIG_RISING(false);
	 ADS7843_INT_IRQ_CONFIG_FALLING(true);
	 m_touchInfoData.touchStatus = TOUCH_STATUS_PENUP;
	 }
	 }
	 */
}

bool ADS7843getIrqPinState(void) {
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

void ADS7843readXY(uint16_t *x, uint16_t *y) {
	uint16_t xyDataBuf[2][TOUCH_MAX_NUM_OF_SAMPLES];
	uint8_t i, j;
	uint16_t temp;

	for (i = 0; i < 7; i++) {
		ADS7843readRawXY(&xyDataBuf[0][i], &xyDataBuf[1][i]);
	}
	*x = ADS7843fastMedian(xyDataBuf[0]);
	*y = ADS7843fastMedian(xyDataBuf[1]);
	return;
// Discard the first and the last one of the data and sort remained data
	for (i = 1; i < TOUCH_MAX_NUM_OF_SAMPLES - 2; i++) {
		for (j = i + 1; j < TOUCH_MAX_NUM_OF_SAMPLES - 1; j++) {
			if (xyDataBuf[0][i] > xyDataBuf[0][j]) {
				temp = xyDataBuf[0][i];
				xyDataBuf[0][i] = xyDataBuf[0][j];
				xyDataBuf[0][j] = temp;
			}

			if (xyDataBuf[1][i] > xyDataBuf[1][j]) {
				temp = xyDataBuf[1][i];
				xyDataBuf[1][i] = xyDataBuf[1][j];
				xyDataBuf[1][j] = temp;
			}
		}
	}
	xyDataBuf[0][0] = 0;
	xyDataBuf[1][0] = 0;

// Discard the first and the last one of the sorted data
// and compute the average value of the remained data.
	for (i = 2; i < TOUCH_MAX_NUM_OF_SAMPLES - 2; i++) {
		xyDataBuf[0][0] += xyDataBuf[0][i];
		xyDataBuf[1][0] += xyDataBuf[1][i];
	}
	*x = xyDataBuf[0][0] / (TOUCH_MAX_NUM_OF_SAMPLES - 4);
	*y = xyDataBuf[1][0] / (TOUCH_MAX_NUM_OF_SAMPLES - 4);
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

TouchPoint ADS7843translateCoordinates(const TouchPoint* rawPoint) {
	TouchPoint p;
	p.x = calibCoefs.m_ax * rawPoint->x + calibCoefs.m_bx * rawPoint->y + calibCoefs.m_dx;
	p.y = calibCoefs.m_ay * rawPoint->x + calibCoefs.m_by * rawPoint->y + calibCoefs.m_dy;
	return p;
}

bool ADS7843readPointXY(TouchPoint* touchPoint, bool calibrationEnabled) {
    m_touchInfoData.touchStatus = TOUCH_STATUS_TOUCHING; //TODO to be removed!!
	if (m_touchInfoData.touchStatus == TOUCH_STATUS_TOUCHING) {
		uint16_t xyDataBuf[2][7]; //7 samples
		uint8_t i, j;
		uint16_t temp;
		TouchPoint p;
		for (i = 0; i < 7; i++) {
			ADS7843readRawXY(&xyDataBuf[0][i], &xyDataBuf[1][i]);
		}
		p.x = ADS7843fastMedian(xyDataBuf[0]);
		p.y = ADS7843fastMedian(xyDataBuf[1]);
		if (!calibrationEnabled) {
			*touchPoint = ADS7843translateCoordinates(&p);
		} else {
			*touchPoint = p;
		}
		return true;
	}
	return false;
}

TouchStatus ADS7843getTouchStatus() {
	return m_touchInfoData.touchStatus;
}

void ADS7843setCalibrationCoefficients(const CalibCoefficients* coeffs)
{
	calibCoefs.m_ax = coeffs->m_ax;
	calibCoefs.m_ay = coeffs->m_ay;
	calibCoefs.m_bx = coeffs->m_bx;
	calibCoefs.m_by = coeffs->m_by;
	calibCoefs.m_dx = coeffs->m_dx;
	calibCoefs.m_dy = coeffs->m_dy;

}


bool ADS7843readOnePointRawCoordinates(TouchPoint* point) {
	TouchPoint tempP;
	const uint16_t samplesNum = 100;
	uint16_t idx = 0;
	uint32_t xSum = 0;
	uint32_t ySum = 0;

	while (ADS7843getIrqPinState())
		; //wait till we pen down the panel
	for (;;) {
		if (!ADS7843getIrqPinState()) {
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


