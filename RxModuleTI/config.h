/*
 * config.h
 *
 *  Created on: 19 lis 2016
 *      Author: igbt6
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include "ads7843.h"


#define DEFAULT_VERSION 0xFF

////////////////////////////////////////////////////////////////////////
//FLASH stored parameters
////////////////////////////////////////////////////////////////////////

// Define the size of the flash program blocks for saving configuration data.
#define FLASH_PB_START          0x3FF00
#define FLASH_PB_END            FLASH_PB_START + 0xFF
//#define FLASH_PB_START          0x20000
//#define FLASH_PB_END            FLASH_PB_START + 0x4000
// The size of the parameter block to save.  This must be a power of 2,
// and should be large enough to contain the tConfigParameters structure.
#define FLASH_PB_SIZE           256

typedef struct
{
	uint8_t paramsVersion;
	uint8_t isModified;
}ConfigFlashParameters;

void configFlashInit(void);
void configFlashLoadFactory(void);
void configFlashLoad(void);
void configFlashSave(void);
ConfigFlashParameters* configFlashGetCurrent(void);

////////////////////////////////////////////////////////////////////////
//EEPROM stored parameters
////////////////////////////////////////////////////////////////////////
#define MAX_CITIES 3
#define MAX_PARAMETER_NAME_LENGTH 20
#define MAX_WIFI_CONFIGS 2

#define  EEPROM_START_ADDRESS 0x0000

typedef struct
{
	CalibCoefficients calibCoeffs;
	uint8_t isModified;
}TouchScreenConfigParameters;

typedef struct
{
	char wifiSSID[MAX_PARAMETER_NAME_LENGTH];
	char wifiWPA2pass[MAX_PARAMETER_NAME_LENGTH];
	uint8_t updatePeriodTime; /*Time after which  request is sent in seconds*/
}AccessPointConfigParameters;

typedef struct
{
	TouchScreenConfigParameters touchScreenParams;
	AccessPointConfigParameters wifiConfig[MAX_WIFI_CONFIGS];
	char cityNames[MAX_CITIES][MAX_PARAMETER_NAME_LENGTH];
	char openweatherDomain[MAX_PARAMETER_NAME_LENGTH + MAX_PARAMETER_NAME_LENGTH];
	uint8_t paramsVersion;
	uint8_t isModified;
}ConfigEepromParameters;

/*
 * @brief utils for EEprom memory
 * @return true on success
 */
void configEepromLoad(void);
bool configEepromInit(void);
bool configEepromSaveDefaults(void);
bool configEepromSave(void);
ConfigEepromParameters* configEepromGetCurrent(void);

////////////////////////////////////////////////////////////////////////
//public API methods:
////////////////////////////////////////////////////////////////////////
void configInit(void);

#endif /* CONFIG_H_ */
