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
#include "gpiointerrupt/gpiointerrupt.h"
#include "bsp.h"
#include "em_cmu.h"
#include "../emdrv/spidrv/spidrv.h"


//*****************************************************************************
//privates responsible SPI communication and IRQ handling
//TODO - put it into the class
//*****************************************************************************
static SPIDRV_HandleData_t handleData;
static SPIDRV_Handle_t handle = &handleData;

static void TransferComplete(SPIDRV_Handle_t handle, Ecode_t transferStatus,
		int itemsTransferred) {
	if (transferStatus == ECODE_EMDRV_SPIDRV_OK)
	{
		return;
	}
}

static uint16_t spiReadData(uint8_t reg)
{
	uint8_t txBuf = reg;
	uint8_t rxBuf[2] = { 0, 0 };
	uint16_t cur = 0x0000;
	//SPIDRV_MTransmit( handle, txBuf, 8,TransferComplete );
	ADS7843_CS_LOW();
	SPIDRV_MTransmitB(handle, &txBuf, 1);
	SPIDRV_MReceiveB(handle, rxBuf, 2);
	cur = ((rxBuf[0] << 4) | (rxBuf[1] >> 4)) & 0x0FFF;
	ADS7843_CS_HIGH();
	return cur;
}



#define ADS7843_ENABLE_TOUCH_INT


ADS7843::ADS7843():m_pinIrqCb(nullptr)
{
	init();
}

ADS7843::~ADS7843()
{
	delete m_pinIrqCb;
	m_pinIrqCb = nullptr;
}

void ADS7843::init(void)
{
    /** Create a Callback with a member function
     *  @param obj  Pointer to object to invoke member function on
     *  @param func Member function to attach
     */
	/// Configuration data for SPI master using USART1.
#define SPIDRV_MASTER_USART1_LOC1                                         \
	{                                                                         \
	  USART1,                       /* USART port                       */    \
	  _USART_ROUTE_LOCATION_LOC1,   /* USART pins location number       */    \
	  1000000,                      /* Bitrate                          */    \
	  8,                            /* Frame length                     */    \
	  0,                            /* Dummy tx value for rx only funcs */    \
	  spidrvMaster,                 /* SPI mode                         */    \
	  spidrvBitOrderMsbFirst,       /* Bit order on bus                 */    \
	  spidrvClockMode0,             /* SPI clock/phase mode             */    \
	  spidrvCsControlApplication,   /* CS controlled by the driver      */    \
	  spidrvSlaveStartImmediate     /* Slave start transfers immediately*/    \
	}

	CMU_ClockEnable(cmuClock_HFPER, true);
	// Enable clock for USART1
	CMU_ClockEnable(cmuClock_USART1, true);
	CMU_ClockEnable(cmuClock_GPIO, true);
	ADS7843_MOSI_OUTPUT();
	ADS7843_MISO_INPUT();
	ADS7843_CLK_OUTPUT();
	ADS7843_CS_OUTPUT();
	SPIDRV_Init_t initData = SPIDRV_MASTER_USART1_LOC1;
	// Initialize a SPI driver instance
	SPIDRV_Init(handle, &initData);

#ifdef ADS7843_USE_PIN_BUSY
	ADS7843_BUSY_INPUT();
#endif

#ifdef ADS7843_ENABLE_TOUCH_INT
	// Initialize GPIO interrupt dispatcher
	GPIOINT_Init();
	ADS7843_INT_INPUT();
	m_pinIrqCb = new Callback<void(uint8_t)>(this,&ADS7843::penIRQCallback);
	GPIOINT_CallbackRegister(ADS7843_PIN_INT, m_pinIrqCb);
	ADS7843_INT_IRQ_CONFIG_FALLING(true); //falling edge - reacts on PENDOWN touch
#endif
	// assign default values
	m_touchInfoData.thAdRight = TOUCH_AD_X_MAX;
	m_touchInfoData.thAdLeft = TOUCH_AD_X_MIN;
	m_touchInfoData.thAdUp = TOUCH_AD_Y_MAX;
	m_touchInfoData.thAdDown = TOUCH_AD_Y_MIN;
	m_touchInfoData.touchStatus = TOUCH_STATUS_PENUP;
	setIrqAndPowerDown();
}

void ADS7843::setIrqAndPowerDown(void)
{
	uint8_t buf[3] = { ADS7843_READ_X | ADS7843_DFR, 0, 0 };
	ADS7843_CS_LOW();
	SPIDRV_MTransmitB(handle, buf, 1);
	SPIDRV_MTransmitB(handle, &buf[1], 2);
	ADS7843_CS_HIGH();
}

void ADS7843::penIRQCallback(uint8_t pin)
{
	if (pin == ADS7843_PIN_INT)
	{
		if(!getIrqPinState()) //if pendown
		{
		//ADS7843_INT_IRQ_CONFIG_PIN_DISABLE();
			ADS7843_INT_IRQ_CONFIG_FALLING(false);
			ADS7843_INT_IRQ_CONFIG_RISING(true);
			m_touchInfoData.touchStatus = TOUCH_STATUS_TOUCHING;
		}
		else
		{
			ADS7843_INT_IRQ_CONFIG_RISING(false);
			ADS7843_INT_IRQ_CONFIG_FALLING(true);
			m_touchInfoData.touchStatus = TOUCH_STATUS_PENUP;
		}
	}
}


bool ADS7843::getIrqPinState(void)
{
	return (bool) ADS7843_GET_INT_PIN();
}



void ADS7843::readRawXY(uint16_t *x, uint16_t *y) {
	*x = spiReadData(ADS7843_READ_X | ADS7843_DFR);
#ifdef ADS7843_USE_PIN_BUSY
	// wait for conversion complete
	while(ADS7843_GET_BUSY_PIN());
#else
	//for (volatile int i = 0; i < 1000; i++);
#endif
	*y =spiReadData(ADS7843_READ_Y | ADS7843_DFR);
}


void ADS7843::readXY(uint16_t *x, uint16_t *y)
{
	uint16_t xyDataBuf[2][TOUCH_MAX_NUM_OF_SAMPLES];
	uint8_t i, j;
	uint16_t temp;

	for (i = 0; i < 7; i++)
	{
		readRawXY(&xyDataBuf[0][i], &xyDataBuf[1][i]);
	}
    *x=fastMedian(xyDataBuf[0]);
    *y=fastMedian(xyDataBuf[1]);
    return;
// Discard the first and the last one of the data and sort remained data
	for (i = 1; i < TOUCH_MAX_NUM_OF_SAMPLES - 2; i++)
	{
		for (j = i + 1; j < TOUCH_MAX_NUM_OF_SAMPLES - 1; j++)
		{
			if (xyDataBuf[0][i] > xyDataBuf[0][j])
			{
				temp = xyDataBuf[0][i];
				xyDataBuf[0][i] = xyDataBuf[0][j];
				xyDataBuf[0][j] = temp;
			}

			if (xyDataBuf[1][i] > xyDataBuf[1][j])
			{
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
	for (i = 2; i < TOUCH_MAX_NUM_OF_SAMPLES - 2; i++)
	{
		xyDataBuf[0][0] += xyDataBuf[0][i];
		xyDataBuf[1][0] += xyDataBuf[1][i];
	}
	*x = xyDataBuf[0][0] / (TOUCH_MAX_NUM_OF_SAMPLES - 4);
	*y = xyDataBuf[1][0] / (TOUCH_MAX_NUM_OF_SAMPLES - 4);
}


uint16_t ADS7843::fastMedian(uint16_t *samples) const
{
  // do a fast median selection (reference code available on internet). This code basically
  // avoids sorting the entire samples array
  #define PIX_SORT(a,b) { if ((a)>(b)) PIX_SWAP((a),(b)); }
  #define PIX_SWAP(a,b) { uint16_t temp=(a);(a)=(b);(b)=temp; }
  PIX_SORT(samples[0], samples[5]) ; PIX_SORT(samples[0], samples[3]) ; PIX_SORT(samples[1], samples[6]) ;
  PIX_SORT(samples[2], samples[4]) ; PIX_SORT(samples[0], samples[1]) ; PIX_SORT(samples[3], samples[5]) ;
  PIX_SORT(samples[2], samples[6]) ; PIX_SORT(samples[2], samples[3]) ; PIX_SORT(samples[3], samples[6]) ;
  PIX_SORT(samples[4], samples[5]) ; PIX_SORT(samples[1], samples[4]) ; PIX_SORT(samples[1], samples[3]) ;
  PIX_SORT(samples[3], samples[4]) ;
  return samples[3];
}


void ADS7843::readXYMedian(TouchPoint& touchPoint,bool cal)
{
	uint16_t xyDataBuf[2][7]; //7 samples
	uint8_t i, j;
	uint16_t temp;
	TouchPoint p;
	for (i = 0; i < 7; i++)
	{
		readRawXY(&xyDataBuf[0][i], &xyDataBuf[1][i]);
	}
    p.x = fastMedian(xyDataBuf[0]);
    p.y = fastMedian(xyDataBuf[1]);
    //p.y = TOUCH_AD_Y_MAX -p.y;
    //if(p.y > TOUCH_AD_Y_MAX)
    //	p.y=0;
    //p.y = TOUCH_SCREEN_HEIGHT - p.y; //for portrait
    if(!cal)
    	touchPoint = translateCoordinates(p);
    else
    	touchPoint = p;
}

ADS7843::TouchPoint ADS7843::translateCoordinates(const TouchPoint& rawPoint)
{
  TouchPoint p;
  p.x=m_ax*rawPoint.x + m_bx*rawPoint.y + m_dx;
  p.y= m_ay*rawPoint.x + m_by*rawPoint.y + m_dy;
  return p;
}

bool ADS7843::readPointXY(uint16_t *x, uint16_t *y)
{
	uint16_t adX;
	uint16_t adY;
	uint32_t diff = 0;

	if (m_touchInfoData.touchStatus == TOUCH_STATUS_TOUCHING)
	{
		//  If pen down
		readXY(&adX, &adY);
		//x
		diff = m_touchInfoData.thAdRight - m_touchInfoData.thAdLeft;
		//  limit the edges
		if (adX < (m_touchInfoData.thAdLeft - TOUCH_AD_CALIB_ERROR)
				|| adX > (m_touchInfoData.thAdRight + TOUCH_AD_CALIB_ERROR))
		{
			adX = 0;
		}
		//  calculate the x coordinate
		*x = (adX * TOUCH_SCREEN_WIDTH) / diff;
		if(*x > TOUCH_SCREEN_WIDTH)
		{
			*x = TOUCH_SCREEN_WIDTH-1;
		}
		m_touchInfoData.lastX = m_touchInfoData.curX;
		m_touchInfoData.curX = *x;

		//y
		diff = m_touchInfoData.thAdUp - m_touchInfoData.thAdDown;
		//  limit the edges
		if (adY < (m_touchInfoData.thAdDown - TOUCH_AD_CALIB_ERROR)
				|| adY > (m_touchInfoData.thAdUp + TOUCH_AD_CALIB_ERROR))
		{
			adY = 0;
		}
		//  calculate the y coordinate
		*y = (adY * TOUCH_SCREEN_HEIGHT) / diff;
		if(*y > TOUCH_SCREEN_HEIGHT)
		{
			*y = TOUCH_SCREEN_HEIGHT-1;
		}
		*y=TOUCH_SCREEN_HEIGHT -*y; //for portrait
		m_touchInfoData.lastY = m_touchInfoData.curY;
		m_touchInfoData.curY = *y;
		return true;
	}
	return false;
}

void ADS7843::getTouchPointCoordinates(uint16_t* x, uint16_t* y)
{
	*x=m_touchInfoData.curX;
	*y=m_touchInfoData.curY;
}



ADS7843::TouchStatus ADS7843::getTouchStatus()
{
	return m_touchInfoData.touchStatus;
}


uint8_t ADS7843::calibration(ILI9320& lcd)
{
	TouchPoint p1,p2,p3;
	TouchPoint t1,t2,t3;
	TouchPoint tempP;
	uint32_t xSum,ySum;
	uint16_t tempX, tempY;
	// point 1 is at 25%,50%, 2 is at 75%,25% and 3 is at 75%,75%
	ILI9320::Resolution res = lcd.getResolution();
	p1.x= (res.x *25) /100;
	p1.y= (res.y *50) /100;
	p2.x= (res.x *75) /100;
	p2.y= (res.y *25) /100;
	p3.x= (res.x *75) /100;
	p3.y= (res.y *75) /100;

	ADS7843_INT_IRQ_CONFIG_FALLING(false);
	ADS7843_INT_IRQ_CONFIG_RISING(false);
	//1st point
	lcd.ILI9320print("Tap and hold the point", 5, 10, 0, ILI9320::Colors::RED);
	lcd.ILI9320drawCircle(p1.x,p1.y,30, ILI9320::Colors::BLACK);
	lcd.ILI9320fillCircle(p1.x,p1.y,10, ILI9320::Colors::BLACK);
	while(getIrqPinState()); //wait till we pen down the panel
    // get sampling 100 points
    xSum=0;
    ySum=0;
    int i =0;
    for(;;)
	{
    	if(!getIrqPinState())
    	{
			readXYMedian(tempP,true);
			xSum+=tempP.x;
			ySum+=tempP.y;
			i++;
			if(i==100)
				break;
    	}
	}
    t1.x = xSum/100;
    t1.y = ySum/100;
	lcd.ILI9320drawCircle(p1.x,p1.y,30, ILI9320::Colors::GREEN);
	lcd.ILI9320fillCircle(p1.x,p1.y,10, ILI9320::Colors::GREEN);
    Delay(2000000); //2s
    //2nd point
	lcd.ILI9320print("Tap and hold the point", 5, 10, 0, ILI9320::Colors::RED);
    lcd.ILI9320drawCircle(p2.x,p2.y,30, ILI9320::Colors::BLACK);
	lcd.ILI9320fillCircle(p2.x,p2.y,10, ILI9320::Colors::BLACK);
	while(getIrqPinState()); //wait till we pen down the panel
    xSum=0;
    ySum=0;
    i =0;
    for(;;)
	{
    	if(!getIrqPinState())
    	{
			readXYMedian(tempP,true);
			xSum+=tempP.x;
			ySum+=tempP.y;
			i++;
			if(i==100)
				break;
    	}
	}
    t2.x = xSum/100;
    t2.y = ySum/100;
	lcd.ILI9320drawCircle(p2.x,p2.y,30, ILI9320::Colors::GREEN);
	lcd.ILI9320fillCircle(p2.x,p2.y,10, ILI9320::Colors::GREEN);
    Delay(2000000); //2s
    //3rd point
	lcd.ILI9320print("Tap and hold the point", 5, 10, 0, ILI9320::Colors::RED);
    lcd.ILI9320drawCircle(p3.x,p3.y,30, ILI9320::Colors::BLACK);
    lcd.ILI9320fillCircle(p3.x,p3.y,10, ILI9320::Colors::BLACK);
	while(getIrqPinState()); //wait till we pen down the panel
    xSum=0;
    ySum=0;
    i =0;
    for(;;)
	{
    	if(!getIrqPinState())
    	{
			readXYMedian(tempP,true);
			xSum+=tempP.x;
			ySum+=tempP.y;
			i++;
			if(i==100)
				break;
    	}
	}
    t3.x = xSum/100;
    t3.y = ySum/100;
	lcd.ILI9320drawCircle(p3.x,p3.y,30, ILI9320::Colors::GREEN);
	lcd.ILI9320fillCircle(p3.x,p3.y,10, ILI9320::Colors::GREEN);
    Delay(2000000); //2s
    //final computation based on: https://www.maximintegrated.com/en/app-notes/index.mvp/id/5296
    int32_t delta,deltaX1,deltaX2,deltaX3,deltaY1,deltaY2,deltaY3;

         // intermediate values for the calculation
    delta=((int32_t)(t1.x-t3.x)*(int32_t)(t2.y-t3.y))-((int32_t)(t2.x-t3.x)*(int32_t)(t1.y-t3.y));

    deltaX1=((int32_t)(p1.x-p3.x)*(int32_t)(t2.y-t3.y))-((int32_t)(p2.x-p3.x)*(int32_t)(t1.y-t3.y));
    deltaX2=((int32_t)(t1.x-t3.x)*(int32_t)(p2.x-p3.x))-((int32_t)(t2.x-t3.x)*(int32_t)(p1.x-p3.x));
    deltaX3=p1.x*((int32_t)t2.x*(int32_t)t3.y - (int32_t)t3.x*(int32_t)t2.y) -
            p2.x*(t1.x*(int32_t)t3.y - (int32_t)t3.x*(int32_t)t1.y) +
            p3.x*((int32_t)t1.x*(int32_t)t2.y - (int32_t)t2.x*(int32_t)t1.y);

    deltaY1=((int32_t)(p1.y-p3.y)*(int32_t)(t2.y-t3.y))-((int32_t)(p2.y-p3.y)*(int32_t)(t1.y-t3.y));
    deltaY2=((int32_t)(t1.x-t3.x)*(int32_t)(p2.y-p3.y))-((int32_t)(t2.x-t3.x)*(int32_t)(p1.y-p3.y));
    deltaY3=p1.y*((int32_t)t2.x*(int32_t)t3.y - (int32_t)t3.x*(int32_t)t2.y) -
            p2.y*((int32_t)t1.x*(int32_t)t3.y - (int32_t)t3.x*(int32_t)t1.y) +
            p3.y*((int32_t)t1.x*(int32_t)t2.y - (int32_t)t2.x*t1.y);

     // final values
	 m_ax=(float)deltaX1/(float)delta;
	 m_bx=(float)deltaX2/(float)delta;
	 m_dx=(float)deltaX3/(float)delta;

	 m_ay=(float)deltaY1/(float)delta;
	 m_by=(float)deltaY2/(float)delta;
	 m_dy=(float)deltaY3/(float)delta;
	 lcd.ILI9320clrXY();
	 ADS7843_INT_IRQ_CONFIG_FALLING(true);

	return 0;
}
