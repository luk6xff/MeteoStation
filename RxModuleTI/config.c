/*
 * config.c
 *
 *  Created on: 19 lis 2016
 *      Author: igbt6
 */

#include "config.h"
#include "utils/flash_pb.h"
#include "driverlib/debug.h"


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


