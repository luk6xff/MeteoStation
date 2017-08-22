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

#include "system.h"
#include "config.h"

////////////////////////////////////////////////////////////////////////
// FLASH
////////////////////////////////////////////////////////////////////////
static const ConfigFlashParameters default_flash_settings =
{
	{
		0x00,	// wifi_enabled
		0x00,	// sensorsEnabled
		0x00,	// powerSavingEnabled
		0x00,	// wifiConnectionState - WIFI_NOT_CONNECTED
		0x00	// sensorConnectionState - SENSOR_NOT_CONNECTED
	},
	0x01, // params_version
	0x00  // is_modified
};


static ConfigFlashParameters m_current_flash_parameters;

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
    ASSERT(sizeof(ConfigFlashParameters) == FLASH_PB_SIZE);
    // Initialize the flash parameter block driver.
    FlashPBInit(FLASH_PB_START, FLASH_PB_END, FLASH_PB_SIZE);
}

void configFlashLoadFactory(void)
{
	m_current_flash_parameters = default_flash_settings;
}

void configFlashLoad(void)
{
	uint8_t * configBuffer;

	//Get a pointer to the most recently saved config block in the flash.
	configBuffer = FlashPBGet();

	if(configBuffer)
	{
		//copy params from flash to RAM
		m_current_flash_parameters = *(ConfigFlashParameters*)configBuffer;
	}
}

void configFlashSaveDefaults(void)
{
	FlashPBSave((uint8_t*)&default_flash_settings);
}

void configFlashSave(void)
{
	FlashPBSave((uint8_t*)&m_current_flash_parameters);
}

void configFlashSetModified(ConfigFlashParameters* const ptr)
{
	ptr->is_modified = 0x01;
}

bool configFlashCheckAndCleanModified(ConfigFlashParameters * const ptr)
{
	if(ptr && ptr->is_modified == 0x01)
	{
		ptr->is_modified = 0x00; //clear
		return true;
	}
	return false;
}

bool configFlashSetInvalid(ConfigFlashParameters* const ptr)
{
	if(!ptr)
	{
		return false;
	}
	ptr->params_version = 0xFF;
	return true;
}

bool configFlashIsInvalid(const ConfigFlashParameters * const ptr)
{
	return (ptr->params_version == 0xFF || ptr->params_version == 0x00);
}

ConfigFlashParameters* const configFlashGetDefaultSettings(void)
{
	return &default_flash_settings;
}

ConfigFlashParameters* configFlashGetCurrent(void)
{
	return &m_current_flash_parameters;
}


////////////////////////////////////////////////////////////////////////
// EEPROM
////////////////////////////////////////////////////////////////////////
static const ConfigEepromParameters default_eeprom_settings =
{
		{
				{0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f},	// calibCoeffs
				0x00,  									// isValid
		},
		.wifi_config = {
				{"default"}, 							// apSSID
				{"default"},						    // apPass
		},
		{"city"},										// cityName

		60,												// updateWifiPeriodTime
		60,												// updateSensorPeriodTime
		0x01, 											// params_version 0x00 and 0xFF means invalid one
		0x00											// is_modified
};

static ConfigEepromParameters m_current_eeprom_parameters;

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
	EEPROMRead((uint8_t*)&m_current_eeprom_parameters, EEPROM_START_ADDRESS, sizeof(ConfigEepromParameters));
}

bool configEepromSaveDefaults(void)
{
	return 0 == EEPROMProgram((uint8_t*)&default_eeprom_settings, EEPROM_START_ADDRESS, sizeof(ConfigEepromParameters));
}

bool configEepromSave(void)
{
	return 0 == EEPROMProgram((uint8_t*)&m_current_eeprom_parameters, EEPROM_START_ADDRESS, sizeof(ConfigEepromParameters));
}

void configEepromSetModified(ConfigEepromParameters* const ptr)
{
	ptr->is_modified = 0x01;
}

bool configEepromCheckAndCleanModified(ConfigEepromParameters * const ptr)
{
	if(ptr && ptr->is_modified == 0x01)
	{
		ptr->is_modified = 0x00; //clear
		return true;
	}
	return false;
}

bool configEepromSetInvalid(ConfigEepromParameters * const ptr)
{
	if(!ptr)
	{
		return false;
	}
	ptr->params_version = 0xFF;
	return true;
}

bool configEepromIsInvalid(const ConfigEepromParameters * const ptr)
{
	return (ptr->params_version == 0xFF || ptr->params_version == 0x00);
}

ConfigEepromParameters* const configEepromGetDefaultSettings(void)
{
	return &default_eeprom_settings;
}

ConfigEepromParameters* configEepromGetCurrent(void)
{
	return &m_current_eeprom_parameters;
}

//@brief Global save methods
bool configFlashSaveSettingsToMemory(const ConfigFlashParameters* settings)
{
	DISABLE_ALL_INTERRUPTS();
	bool ret = false;
	if(configFlashCheckAndCleanModified(settings))
	{
		memcpy(configFlashGetCurrent(), settings, sizeof(ConfigFlashParameters));
		configFlashSave();
		ret = true;
	}
	ENABLE_ALL_INTERRUPTS();
	return ret;
}

bool configEepromSaveSettingsToMemory(const ConfigEepromParameters* settings)
{
	DISABLE_ALL_INTERRUPTS();
	bool ret = false;
	if(configEepromCheckAndCleanModified(settings))
	{
		memcpy(configEepromGetCurrent(), settings, sizeof(ConfigEepromParameters));
		configEepromSave();
		ret = true;
	}
	ENABLE_ALL_INTERRUPTS();
	return ret;
}

