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
#define FLASH_PB_START          0x3FC00
#define FLASH_PB_END            FLASH_PB_START + 0x3FF
// The size of the parameter block to save.  This must be a power of 2,
// and should be large enough to contain the ConfigFlashParameters structure.
#define FLASH_PB_SIZE           64 // 16 blocks 128B of 1KB

typedef struct
{
	uint8_t wifiEnabled;
	uint8_t sensorsEnabled;
	uint8_t powerSavingEnabled;
	uint8_t wifiConnectionState;
	uint8_t sensorConnectionState;
}ConectionSetupState;

typedef struct
{
	ConectionSetupState connectionSetupState;
	uint8_t params_version;
	uint8_t is_modified;
}ConfigFlashParameters;

void configFlashInit(void);
void configFlashLoadFactory(void);
void configFlashLoad(void);
void configFlashSaveDefaults(void);
void configFlashSave(void);
void configFlashSetModified(ConfigFlashParameters* const ptr);
bool configFlashCheckAndCleanModified(ConfigFlashParameters * const ptr);
bool configFlashSetInvalid(ConfigFlashParameters* const ptr);
bool configFlashIsInvalid(const ConfigFlashParameters * const ptr);
ConfigFlashParameters* const configFlashGetDefaultSettings(void);
ConfigFlashParameters* configFlashGetCurrent(void);

////////////////////////////////////////////////////////////////////////
//EEPROM stored parameters
////////////////////////////////////////////////////////////////////////
#define CONFIG_MAX_PARAM_NAME_LENGTH 20

#define  EEPROM_START_ADDRESS 0x0000

typedef struct
{
	CalibCoefficients calib_coeffs;
	uint8_t is_valid; // 0 - default invalid, other - valid
}TouchScreenConfigParameters;

typedef struct
{
	char ap_ssid[CONFIG_MAX_PARAM_NAME_LENGTH];
	char ap_wpa2_pass[CONFIG_MAX_PARAM_NAME_LENGTH];
}AccessPointConfigParameters;

typedef struct
{
	TouchScreenConfigParameters touch_screen_params;
	AccessPointConfigParameters wifi_config;
	char city_name[CONFIG_MAX_PARAM_NAME_LENGTH];
	uint8_t update_wifi_period_time;  /*Time after which request to OpenWeatherMap is sent in seconds*/
	uint8_t update_sensor_period_time;/*Time after which request to Sensor is sent in seconds*/
	uint8_t params_version; 		   /* First Time the value will be 0x00 -invalid, 0x01-defaults, 0x02 - updated */
	uint8_t is_modified;
}ConfigEepromParameters;

/*
 * @brief utils for EEprom memory
 * @return true on success
 */

void configEepromLoad(void);
bool configEepromInit(void);
bool configEepromSaveDefaults(void);
bool configEepromSave(void);
void configEepromSetModified(ConfigEepromParameters* const ptr);
bool configEepromCheckAndCleanModified(ConfigEepromParameters * const ptr);
bool configEepromSetInvalid(ConfigEepromParameters * const ptr);
bool configEepromIsInvalid(const ConfigEepromParameters * const ptr);
ConfigEepromParameters* const configEepromGetDefaultSettings(void);
ConfigEepromParameters* configEepromGetCurrent(void);

////////////////////////////////////////////////////////////////////////
//public API methods:
////////////////////////////////////////////////////////////////////////
void configInit(void);

#endif /* CONFIG_H_ */
