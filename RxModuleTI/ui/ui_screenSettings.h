/*
 * ui_screenSettings.h
 *
 *  Created on: 2 lip 2017
 *      Author: igbt6
 */

#ifndef UI_UI_SCREENSETTINGS_H_
#define UI_UI_SCREENSETTINGS_H_

#include "../config.h"

void uiScreenSettings_registerConnectionSetupState(ConectionSetupState* state);

void uiScreenSettings_registerOnConnToAP(void (*OnConnToAppCb)(void));

void uiScreenSettings_registerParams(char* cityPtr, char* passPtr, char* ssidPtr, char* timeEntryPtr);

void uiScreenSettings_init(void);

void uiScreenSettingsUpdate(void);
#endif /* UI_UI_SCREENSETTINGS_H_ */
