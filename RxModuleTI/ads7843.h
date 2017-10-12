/*
 * ads7843.h
 *
 *  Created on: 08-08-2016
 *      Author: igbt6
 */

#ifndef ADS7843_H_
#define ADS7843_H_

#include "system.h"
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


//------------------SSI0 ADS7843 CS_PIN----------------------
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
