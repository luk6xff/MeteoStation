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
		0x00,
		0x00,
		0x00,
		0x00,
	},
	0xFF,
	0x0
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

void configFlashSave(void)
{
	FlashPBSave((uint8_t*)&m_currentFlashParameters);
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
				{0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f},
				0x00
		},
		.wifiConfig[0] = {
				{"default"},
				{"defaultPass"},
		},
		.wifiConfig[1] = {
				{"default"},
				{"defaultPass"},
		},
		{
				{""}, {""}, {""}
		},
		.openweatherDomain = {"https://api.openweathermap.org/"},
		0xFF, /*0x00 and 0xFF means invalid one*/
		0x00
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


ConfigEepromParameters* configEepromGetCurrent(void)
{
	return &m_currentEepromParameters;
}


