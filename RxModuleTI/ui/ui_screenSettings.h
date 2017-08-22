/*
 * ui_screenSettings.h
 *
 *  Created on: 2 lip 2017
 *      Author: igbt6
 */

#ifndef UI_UI_SCREENSETTINGS_H_
#define UI_UI_SCREENSETTINGS_H_

#include "../config.h"

void uiScreenSettings_init(ConfigEepromParameters* eepromConfig, ConfigFlashParameters* flashConfig);

void uiScreenSettings_update(void);

void uiScreenSettings_onExit(void);
#endif /* UI_UI_SCREENSETTINGS_H_ */
