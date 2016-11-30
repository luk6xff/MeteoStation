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
// FLASH
static const ConfigParameters defaultSettings =
{
		{
				{0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f},
				0x00
		},
#ifdef ALL_PARAMS_SUPPORTED
		.wifiConfig = {
				{"default"},
				{"default_pass"},

				{0xFFFFFFFF}
		},
		{
				{""}, {""}, {""}, {""}, {""}
		},
		.openweatherDomain = {"https://openweathermap.org/"},
#endif
		0xFF,
		0x00
};



static ConfigParameters m_currentParameters;

void configInit(void)
{
    // Verify that the parameter block structure matches the FLASH parameter
    // block size.
    ASSERT(sizeof(ConfigParameters) == FLASH_PB_SIZE);

    // Initialize the flash parameter block driver.
    FlashPBInit(FLASH_PB_START, FLASH_PB_END, FLASH_PB_SIZE);

    configLoadFactory();

    configLoad();

    configEepromInit();
    configEepromLoad();

}

void configLoadFactory(void)
{
	m_currentParameters = defaultSettings;
}

void configLoad(void)
{
	uint8_t * configBuffer;

	//Get a pointer to the most recently saved config block in the flash.
	configBuffer = FlashPBGet();

	if(configBuffer)
	{
		//copy params from flash to RAM
		m_currentParameters = *(ConfigParameters*)configBuffer;
	}
}

void configSave(void)
{
	FlashPBSave((uint8_t*)&m_currentParameters);
}

ConfigParameters* configGetCurrent(void)
{
	return &m_currentParameters;
}



// EEPROM
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
void configEepromSave(void)
{
	EEPROMProgram((uint8_t*)&m_currentEepromParameters, EEPROM_START_ADDRESS, sizeof(ConfigEepromParameters));
}


ConfigEepromParameters* configEepromGetCurrent(void)
{
	return &m_currentEepromParameters;
}


