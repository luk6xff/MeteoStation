/*
 * config.c
 *
 *  Created on: 19 lis 2016
 *      Author: igbt6
 */


#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"

#include "utils/flash_pb.h"

#include "driverlib/sysctl.h"
#include "driverlib/debug.h"
#include "driverlib/eeprom.h"

#include "config.h"

////////////////////////////////////////////////////////////////////////
// FLASH
////////////////////////////////////////////////////////////////////////
static const ConfigFlashParameters defaultFlashSettings =
{
	{
		0x00,	// wifiEnabled
		0x00,	// sensorsEnabled
		0x00,	// powerSavingEnabled
		0x00,	// wifiConnectionState - WIFI_NOT_CONNECTED
		0x00	// sensorConnectionState - SENSOR_NOT_CONNECTED
	},
	0x00, // currentCity
	0x00, // currentWifiConfig
	0x01, // paramsVersion
	0x00  // isModified
};


static ConfigFlashParameters m_currentFlashParameters;

void configInit(void)
{
	configFlashInit();
    configFlashLoadFactory();
    configFlashLoad();

    configEepromInit();
    configEepromLoad();
}

void configFlashInit(void)
{
    // Verify that the parameter block structure matches the FLASH parameter
    // block size.
    ASSERT(sizeof(ConfigParameters) == FLASH_PB_SIZE);
    // Initialize the flash parameter block driver.
    FlashPBInit(FLASH_PB_START, FLASH_PB_END, FLASH_PB_SIZE);
}

void configFlashLoadFactory(void)
{
	m_currentFlashParameters = defaultFlashSettings;
}

void configFlashLoad(void)
{
	uint8_t * configBuffer;

	//Get a pointer to the most recently saved config block in the flash.
	configBuffer = FlashPBGet();

	if(configBuffer)
	{
		//copy params from flash to RAM
		m_currentFlashParameters = *(ConfigFlashParameters*)configBuffer;
	}
}

void configFlashSaveDefaults(void)
{
	FlashPBSave((uint8_t*)&defaultFlashSettings);
}

void configFlashSave(void)
{
	FlashPBSave((uint8_t*)&m_currentFlashParameters);
}

bool configFlashCheckAndCleanModified(ConfigFlashParameters * const ptr)
{
	if(ptr && ptr->isModified == 0x01)
	{
		ptr->isModified = 0x00; //clear
		return true;
	}
	return false;
}

void configFlashSetModified(ConfigFlashParameters* const ptr)
{
	ptr->isModified = 0x01;
}

bool configFlashIsInvalid(const ConfigFlashParameters * const ptr)
{
	return (ptr->paramsVersion == 0xFF || ptr->paramsVersion == 0x00);
}

ConfigFlashParameters* const configFlashGetDefaultSettings(void)
{
	return &defaultFlashSettings;
}

ConfigFlashParameters* configFlashGetCurrent(void)
{
	return &m_currentFlashParameters;
}


////////////////////////////////////////////////////////////////////////
// EEPROM
////////////////////////////////////////////////////////////////////////
static const ConfigEepromParameters defaultEepromSettings =
{
		{
				{0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f},	// calibCoeffs
				0x00,  									// isValid
		},
		.wifiConfig[0] = {
				{"default"}, 							// apSSID
				{"defaultPass"},						// isModified
		},
		.wifiConfig[1] = {
				{"default"},
				{"defaultPass"},
		},
		{
				{"city"}, {"city"}, {"city"}			// cityNames
		},
		.openweatherDomain = {"https://api.openweathermap.org/"},
		60,												// updateWifiPeriodTime
		60,												// updateSensorPeriodTime
		0x01, 											// paramsVersion 0x00 and 0xFF means invalid one
		0x00											// isModified
};

static ConfigEepromParameters m_currentEepromParameters;

bool configEepromInit(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_EEPROM0); // EEPROM activate
	if(EEPROMInit() == EEPROM_INIT_OK)
	{
		return true;
	}
	return false;

}

void configEepromLoad(void)
{
	EEPROMRead((uint8_t*)&m_currentEepromParameters, EEPROM_START_ADDRESS, sizeof(ConfigEepromParameters));
}

bool configEepromSaveDefaults(void)
{
	return 0 == EEPROMProgram((uint8_t*)&defaultEepromSettings, EEPROM_START_ADDRESS, sizeof(ConfigEepromParameters));
}

bool configEepromSave(void)
{
	return 0 == EEPROMProgram((uint8_t*)&m_currentEepromParameters, EEPROM_START_ADDRESS, sizeof(ConfigEepromParameters));
}

bool configEepromCheckAndCleanModified(ConfigEepromParameters * const ptr)
{
	if(ptr && ptr->isModified == 0x01)
	{
		ptr->isModified = 0x00; //clear
		return true;
	}
	return false;
}

void configEepromSetModified(ConfigEepromParameters* const ptr)
{
	ptr->isModified = 0x01;
}

bool configEepromIsInvalid(const ConfigEepromParameters * const ptr)
{
	return (ptr->paramsVersion == 0xFF || ptr->paramsVersion == 0x00);
}

ConfigEepromParameters* const configEepromGetDefaultSettings(void)
{
	return &defaultEepromSettings;
}

ConfigEepromParameters* configEepromGetCurrent(void)
{
	return &m_currentEepromParameters;
}


