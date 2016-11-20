/*
 * config.h
 *
 *  Created on: 19 lis 2016
 *      Author: igbt6
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include "ads7843.h"



#define MAX_CITIES 1
#define MAX_CITY_NAME_LENGTH 10
#define MAX_WIFI_CONFIGS 1

#define DEFAULT_VERSION 0xFF

//*****************************************************************************
//
// Define the size of the flash program blocks for saving configuration data.
//
//*****************************************************************************
#define FLASH_PB_START          0x3FF00
#define FLASH_PB_END            FLASH_PB_START + 0xFF
//#define FLASH_PB_START          0x20000
//#define FLASH_PB_END            FLASH_PB_START + 0x4000

//*****************************************************************************
//
//! The size of the parameter block to save.  This must be a power of 2,
//! and should be large enough to contain the tConfigParameters structure.
//
//*****************************************************************************
#define FLASH_PB_SIZE           256

#define ALL_PARAMS_SUPPORTEDx

typedef struct
{
	CalibCoefficients calibCoeffs;
	uint8_t isUpdated;
}TouchScreenConfigParameters;

typedef struct
{
	char wifiSSID[20];
	char wifiWPA2_key[20];
	uint32_t ipAddr;
}AccessPointConfigParameters;

typedef struct
{
	TouchScreenConfigParameters touchScreenParams;
#ifdef ALL_PARAMS_SUPPORTED
	AccessPointConfigParameters wifiConfig[MAX_WIFI_CONFIGS];
	char cityNames[MAX_CITIES][MAX_CITY_NAME_LENGTH];
	char openweatherDomain[50];
#endif
	uint8_t paramsVersion;
	uint8_t isModified;

}ConfigParameters;


void configInit(void);
void configLoadFactory(void);
void configLoad(void);
void configSave(void);
ConfigParameters* configGetCurrent(void);


#endif /* CONFIG_H_ */
