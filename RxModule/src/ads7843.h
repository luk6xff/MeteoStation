/*
 * ads7843.h
 *
 *  Created on: 08-08-2016
 *      Author: igbt6
 */

#ifndef ADS7843_H_
#define ADS7843_H_

#include <stdbool.h>
#include "em_device.h"
#include "em_bitband.h"
#include "em_gpio.h"
#include "Callback.h"
#include "ILI9320.h"
#include "LUTouch.h"

/*********************************Hardware dependent part*****************************************/
/*********************************Hardware dependent part*****************************************/

//==================CS=====================
#define ADS7843_PIN_CS      3
#define ADS7843_PORT_CS     gpioPortD
#define ADS7843_CS_OUTPUT() GPIO_PinModeSet(ADS7843_PORT_CS, ADS7843_PIN_CS, gpioModePushPull, 1)
#define ADS7843_CS_HIGH()   GPIO_PinOutSet(ADS7843_PORT_CS , ADS7843_PIN_CS )
#define ADS7843_CS_LOW()    GPIO_PinOutClear(ADS7843_PORT_CS , ADS7843_PIN_CS )

//------------------BUSY_PIN---------------------- TODO!
#define ADS7843_USE_PIN_BUSYx

#ifdef ADS7843_USE_PIN_BUSY
#define ADS7843_PIN_BUSY       6
#define ADS7843_PORT_BUSY      gpioPortD
#define ADS7843_BUSY_OUTPUT()  GPIO_PinModeSet(ADS7843_PORT_BUSY, ADS7843_PIN_BUSY, gpioModePushPull, 1)
#define ADS7843_BUSY_PULLED_INPUT()   GPIO_PinModeSet(ADS7843_PORT_BUSY, ADS7843_PIN_BUSY,  gpioModeInputPull , 1)
#define ADS7843_BUSY_INPUT()   GPIO_PinModeSet(ADS7843_PORT_BUSY, ADS7843_PIN_BUSY, gpioModeInput, 0)
#define ADS7843_BUSY_HIGH()    GPIO_PinOutSet(ADS7843_PORT_BUSY , ADS7843_PIN_BUSY )
#define ADS7843_BUSY_LOW()     GPIO_PinOutClear(ADS7843_PORT_BUSY , ADS7843_PIN_BUSY )
#define ADS7843_GET_BUSY_PIN() GPIO_PinInGet(ADS7843_PORT_BUSY, ADS7843_PIN_BUSY)

#endif

//------------------INT_IRQ----------------------

#define ADS7843_PIN_INT      14
#define ADS7843_PORT_INT     gpioPortA
#define ADS7843_INT_OUTPUT() GPIO_PinModeSet(ADS7843_PORT_INT, ADS7843_PIN_INT, gpioModePushPull, 1)
#define ADS7843_INT_INPUT() GPIO_PinModeSet(ADS7843_PORT_INT, ADS7843_PIN_INT,  gpioModeInputPull , 1)
#define ADS7843_INT_IRQ_CONFIG_FALLING(enable) GPIO_IntConfig(ADS7843_PORT_INT, ADS7843_PIN_INT, false, true, enable)
#define ADS7843_INT_IRQ_CONFIG_RISING(enable) GPIO_IntConfig(ADS7843_PORT_INT, ADS7843_PIN_INT, true, false, enable)
#define ADS7843_INT_IRQ_CONFIG_PIN_DISABLE() GPIO_IntConfig(ADS7843_PORT_INT, ADS7843_PIN_INT, false, false, false)
#define ADS7843_INT_HIGH()   GPIO_PinOutSet(ADS7843_PORT_INT , ADS7843_PIN_INT )
#define ADS7843_INT_LOW()    GPIO_PinOutClear(ADS7843_PORT_INT , ADS7843_PIN_INT )
#define ADS7843_GET_INT_PIN() GPIO_PinInGet(ADS7843_PORT_INT, ADS7843_PIN_INT)

//------------------CLK----------------------

#define ADS7843_PIN_CLK      2
#define ADS7843_PORT_CLK     gpioPortD
#define ADS7843_CLK_OUTPUT() GPIO_PinModeSet(ADS7843_PORT_CLK, ADS7843_PIN_CLK, gpioModePushPull, 0)
#define ADS7843_CLK_HIGH()   GPIO_PinOutSet(ADS7843_PORT_CLK , ADS7843_PIN_CLK )
#define ADS7843_CLK_LOW()    GPIO_PinOutClear(ADS7843_PORT_CLK , ADS7843_PIN_CLK )

//------------------MOSI---------------------

#define ADS7843_PIN_MOSI      0
#define ADS7843_PORT_MOSI     gpioPortD
#define ADS7843_MOSI_OUTPUT() GPIO_PinModeSet(ADS7843_PORT_MOSI, ADS7843_PIN_MOSI, gpioModePushPull, 1)
#define ADS7843_MOSI_HIGH()   GPIO_PinOutSet(ADS7843_PORT_MOSI, ADS7843_PIN_MOSI )
#define ADS7843_MOSI_LOW()    GPIO_PinOutClear(ADS7843_PORT_MOSI , ADS7843_PIN_MOSI )

//------------------MIS0---------------------

#define ADS7843_PIN_MISO      1
#define ADS7843_PORT_MISO     gpioPortD
#define ADS7843_MISO_OUTPUT() GPIO_PinModeSet(ADS7843_PORT_MISO, ADS7843_PIN_MISO, gpioModePushPull, 1)
#define ADS7843_MISO_INPUT()   GPIO_PinModeSet(ADS7843_PORT_MISO, ADS7843_PIN_MISO, gpioModeInput, 0)
#define ADS7843_MISO_HIGH()   GPIO_PinOutSet(ADS7843_PORT_MISO , ADS7843_PIN_MISO )
#define ADS7843_MISO_LOW()    GPIO_PinOutClear(ADS7843_PORT_MISO , ADS7843_PIN_MISO )

/*********************************Hardware dependent part - END*****************************************/

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

class ADS7843 : public LUTouch
{

public:

	typedef enum {
		TOUCH_STATUS_PENUP, TOUCH_STATUS_PENDOWN, TOUCH_STATUS_TOUCHING
	} TouchStatus;

	typedef struct {
		uint16_t lastX;
		uint16_t lastY;
		uint16_t curX;
		uint16_t curY;
		volatile TouchStatus touchStatus;
	} TouchInfo;

	ADS7843();

	~ADS7843();
	//*****************************************************************************
	// @brief Initialize ADS7843
	// @param None.
	// This function initialize ADS7843's SPI interface and .
	// @return None.
	//*****************************************************************************
	void init(void) override;

	//*****************************************************************************
	// @brief reads a current touch point to resu;t container
	// @return If read was succesfull or not.
	//*****************************************************************************
	bool read() override;

	//*****************************************************************************
	// @brief checks if there are data available in the buffer (touchscreen has been enabled)
	// @return true - when touched, false - otherwise.
	//*****************************************************************************
	bool dataAvailable() override;

	//*****************************************************************************
	// @brief gets coordinates of last touched point
	// @return TouchPoint (x,y coordinates)
	//*****************************************************************************
	virtual TouchPoint getTouchedPoint() override;


	//*****************************************************************************
	// @brief Enables IqrPin on startup and enables powerdown between conversions
	//*****************************************************************************
	void setIrqAndPowerDown(void);

	void penIRQCallback(uint8_t pin);

	//****************************************************************************
	// @brief Read the x, y axis ADC convert value once from ADS7843
	// @param x To save the x axis ADC convert value.
	// @param y To save the y axis ADC convert value.
	// @return None.
	//*****************************************************************************
	void readRawXY(uint16_t *x, uint16_t *y);

	//*****************************************************************************
	//@brief read the x, y axis ADC convert value from ADS7843(with software filter)
	//*****************************************************************************
	void readXY(uint16_t *x, uint16_t *y);

	//*****************************************************************************
	// @brief Read the XY coordinate of touch point.
	// @retun true - if read while pendown is touching , false - otherwise
	//*****************************************************************************
	bool readPointXY(TouchPoint& touchPoint, bool calibrationEnabled);

	//*****************************************************************************
	//@brief  Touch screen calibration.
	//*****************************************************************************
	uint8_t performThreePointCalibration(ILI9320& lcd);

	uint16_t fastMedian(uint16_t *samples) const;

    TouchPoint translateCoordinates(const TouchPoint& rawPoint);

	TouchStatus getTouchStatus();

private:

	//*****************************************************************************
	// @brief Get IRQ pin status: false - pen down, true - pen up
	// @retun false - while pen down (LCD is being touched) , true - pen up
	//*****************************************************************************
	bool getIrqPinState(void);

	bool readOnePointRawCoordinates(TouchPoint& point);
private:
	TouchInfo m_touchInfoData;
	Callback<void(uint8_t)> *m_pinIrqCb;
	//corection coefficients
	float m_ax,m_bx,m_dx,m_ay,m_by,m_dy;
};

#endif /* ADS7843_H_ */
