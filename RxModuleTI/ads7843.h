/*
 * ads7843.h
 *
 *  Created on: 08-08-2016
 *      Author: igbt6
 */

#ifndef ADS7843_H_
#define ADS7843_H_

#include <stdbool.h>
#include <stdint.h>

#include "inc/hw_ints.h"

/*********************************Hardware dependent part*****************************************/
/*********************************Hardware dependent part*****************************************/

//------------------BUSY_PIN---------------------- TODO!
#define ADS7843_USE_PIN_BUSYx
#define ADS7843_ENABLE_TOUCH_INT

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


//------------------SSI0 CS_PIN----------------------
#define ADS7843_PIN_CS      			GPIO_PIN_3
#define ADS7843_PORT_CS     			GPIO_PORTA_BASE
#define ADS7843_PORT_CS_CLOCK()			SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA)
#define ADS7843_CS_OUTPUT()  			GPIOPinTypeGPIOOutput(ADS7843_PORT_CS, ADS7843_PIN_CS)
#define ADS7843_CS_HIGH()  				GPIOPinWrite(ADS7843_PORT_CS, ADS7843_PIN_CS, ADS7843_PIN_CS)
#define ADS7843_CS_LOW()  				GPIOPinWrite(ADS7843_PORT_CS, ADS7843_PIN_CS, 0)


//------------------INT_IRQ----------------------
#define ADS7843_PIN_INT      			GPIO_PIN_4
#define ADS7843_PORT_INT     			GPIO_PORTF_BASE
#define ADS7843_INT_INTERRUPT_PORT     	INT_GPIOF
#define ADS7843_PORT_INT_CLOCK()		SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF)
#define ADS7843_INT_INPUT()  			GPIOPinTypeGPIOInput(ADS7843_PORT_INT, ADS7843_PIN_INT);  \
										GPIOPadConfigSet(ADS7843_PORT_INT, ADS7843_PIN_INT, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);  // Enable weak pullup resistor for the pin
#define ADS7843_INT_CONFIG_AS_FALLING() GPIOIntTypeSet(ADS7843_PORT_INT, ADS7843_PIN_INT, GPIO_FALLING_EDGE)
#define ADS7843_INT_CONFIG_AS_RISING()  GPIOIntTypeSet(ADS7843_PORT_INT, ADS7843_PIN_INT, GPIO_RISING_EDGE)
#define ADS7843_INT_INTERRUPT_ENABLE() 	GPIOPinIntEnable(ADS7843_PORT_INT, ADS7843_PIN_INT); IntEnable(ADS7843_INT_INTERRUPT_PORT);
#define ADS7843_INT_INTERRUPT_DISABLE() GPIOPinIntDisable(ADS7843_PORT_INT, ADS7843_PIN_INT); IntDisable(ADS7843_INT_INTERRUPT_PORT);
#define ADS7843_GET_INT_PIN() 			GPIOPinRead(ADS7843_PORT_INT, ADS7843_PIN_INT)


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

typedef struct {
	uint16_t x;
	uint16_t y;
} TouchPoint;

typedef struct
{
	float a_x, b_x, d_x, a_y, b_y, d_y;
}CalibCoefficients;

//*****************************************************************************
// @brief Initialize ADS7843
// @param None.
// This function initialize ADS7843's SPI interface and .
// @return None.
//*****************************************************************************
void ADS7843init(void);

//*****************************************************************************
// @brief reads a current touch point to result container
// @param raw_data_mode - if true data is read in raw mode
// @return If read was succesfull or not.
//*****************************************************************************
bool ADS7843read(bool rawDataMode);

//*****************************************************************************
// @brief checks if there are data available in the buffer (touchscreen has been enabled)
// @return true - when touched, false - otherwise.
//*****************************************************************************
bool ADS7843dataAvailable();

//*****************************************************************************
// @brief gets coordinates of last touched point
// @return TouchPoint (x,y coordinates)
//*****************************************************************************
TouchPoint ADS7843getTouchedPoint();

//*****************************************************************************
// @brief Enables IqrPin on startup and enables powerdown between conversions
//*****************************************************************************
void ADS7843setIrqAndPowerDown(void);


void ADS7843touchPenIntHandler();

//****************************************************************************
// @brief Read the x, y axis ADC convert value once from ADS7843
// @param x To save the x axis ADC convert value.
// @param y To save the y axis ADC convert value.
// @return None.
//*****************************************************************************
void ADS7843readRawXY(uint16_t *x, uint16_t *y);


//*****************************************************************************
// @brief Read the XY coordinate of touch point.
// @param rawDataMode - reads raw data from ADC without Conversion by calib_coefficients
// @retun true - if pendown is pressed, while reading , false - otherwise
//*****************************************************************************
bool ADS7843readPointXY(TouchPoint* touchPoint, bool rawDataMode);


uint16_t ADS7843fastMedian(uint16_t *samples);

TouchPoint ADS7843translateCoordinates(const TouchPoint* rawPoint);

TouchStatus ADS7843getTouchStatus();


void ADS7843setCalibrationCoefficients(const CalibCoefficients* coeffs);
//*****************************************************************************
// @brief Get IRQ pin status: false - pen down, true - pen up
// @retun false - while pen down (LCD is being touched) , true - pen up
//*****************************************************************************
bool ADS7843getIntPinState(void);

bool ADS7843readOnePointRawCoordinates(TouchPoint* point);


#endif /* ADS7843_H_ */
