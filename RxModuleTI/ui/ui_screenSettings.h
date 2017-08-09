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

void uiScreenSettings_registerOnCityEntry(char* cityBuf, void (*OnCityEntryCb)(void));
void uiScreenSettings_registerOnPassEntry(char* passBuf, void (*OnPassEntryCb)(void));
void uiScreenSettings_registerOnSsidEntry(char* ssidBuf, void (*OnSsidEntryCb)(void));
void uiScreenSettings_registerOnUpdateTimeEntry(char* timeEntryBuf, void (*OnUpdateTimeEntryCb)(void));

void uiScreenSettings_registerOnParameterEdited(void (*OnparameterEditedCb)(void));
#endif /* UI_UI_SCREENSETTINGS_H_ */
