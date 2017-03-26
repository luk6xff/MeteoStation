/*
 * system.h
 *
 *  Created on: 25 mar 2017
 *      Author: igbt6
 */

#ifndef SYSTEM_H_
#define SYSTEM_H_

//*****************************************************************************
//
// Defines some settings and configuration available for all modules
//
//*****************************************************************************

#define BG_MIN_X                4
#define BG_MAX_X                (320 - 4)
#define BG_MIN_Y                24
#define BG_MAX_Y                (240 - 8)
#define BG_COLOR_SETTINGS       ClrGray
#define BG_COLOR_MAIN           ClrBlack
#define RED_LED   GPIO_PIN_1
#define BLUE_LED  GPIO_PIN_2
#define GREEN_LED GPIO_PIN_3

//Critical section
//Enable/Disable all interrupts
#define ENABLE_ALL_INTERRUPTS() IntMasterEnable();
#define DISABLE_ALL_INTERRUPTS() IntMasterDisable();



//Screens
//*****************************************************************************
//
// Screens
//
//*****************************************************************************
typedef enum
{
	SCREEN_MAIN,
	SCREEN_CONN_SETTINGS,
	SCREEN_WIFI_SETTINGS,
	SCREEN_SENSOR_SETTINGS,
	SCREEN_KEYBOARD,
	SCREEN_NUM_OF_SCREENS
}Screens;




#endif /* SYSTEM_H_ */


