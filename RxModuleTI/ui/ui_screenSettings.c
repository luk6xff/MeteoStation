/*
 * ui_screenSettings.c
 *
 *  Created on: 2 lip 2017
 *      Author: igbt6
 */
#include "ui_common.h"
#include "ui_screenSettings.h"
#include "ui_keyboard.h"

#include "../system.h"
#include "../wifi.h"

#define UI_SETTINGS_DEBUG_ENABLE 1
#include "../debugConsole.h"
static const char* name = "ui_screenSettings";




static const char* const connStateDesc[] = {
		"NOT_CONNECTED",
		"CONNECTED",
		"Disconnect"
		"Connect",
};


//*****************************************************************************
//
// Methods forward declarations.
//
//*****************************************************************************
static void onConnCheckBoxChange(tWidget *psWidget, uint32_t bSelected);
static void onConnToAP(tWidget *psWidget);
static void onWifiSetup(tWidget *psWidget);
static void onSensorSetup(tWidget *psWidget);
static void onOthersSetup(tWidget *psWidget);

static void onCityEntry(tWidget *psWidget);
static void onSsidEntry(tWidget *psWidget);
static void onPassEntry(tWidget *psWidget);
static void onUpdateTimeEntry(tWidget *psWidget);
static void onParameterEdited(const Screens prevWidget, bool save);


//*****************************************************************************
//
// Local UI state variables / register callbacks methods
//
//*****************************************************************************
static ConfigEepromParameters* eepromConfig;
static ConfigFlashParameters* flashConfig;
static char cityBuf[CONFIG_MAX_PARAM_NAME_LENGTH];
static char passBuf[CONFIG_MAX_PARAM_NAME_LENGTH];
static char ssidBuf[CONFIG_MAX_PARAM_NAME_LENGTH];
static char timeEntryBuf[CONFIG_MAX_PARAM_NAME_LENGTH];

void uiScreenSettings_init(ConfigEepromParameters* eepromConfigPtr, ConfigFlashParameters* flashConfigPtr)
{
	eepromConfig = eepromConfigPtr;
	flashConfig  = flashConfigPtr;
	memcpy(cityBuf, eepromConfigPtr->city_name, sizeof(cityBuf));
	memcpy(passBuf, eepromConfigPtr->wifi_config.ap_wpa2_pass, sizeof(passBuf));
	memcpy(ssidBuf, eepromConfigPtr->wifi_config.ap_ssid, sizeof(ssidBuf));
	//memcpy(timeEntryBuf, eepromConfigPtr->update_wifi_period_time, sizeof(timeEntryBuf));
}

//*****************************************************************************
//
// UI Containers.
//
//*****************************************************************************
tContainerWidget ui_settingsPanelContainers[];
tCanvasWidget ui_settingsCheckBoxIndicators[] =
{
	CanvasStruct
	(
		&ui_settingsPanelContainers,
		ui_settingsCheckBoxIndicators + 1,
		0,
		&g_ILI9320,
		160,
		52,
		20,
		20,
		CANVAS_STYLE_IMG,
		0,
		0,
		0,
		0,
		0,
		img_lightOff,
		0
	),

    CanvasStruct
	(
		&ui_settingsPanelContainers,
		ui_settingsCheckBoxIndicators + 2,
		0,
		&g_ILI9320,
		160,
		92,
		20,
		20,
		CANVAS_STYLE_IMG,
		0,
		0,
		0,
		0,
		0,
		img_lightOff,
		0
	),

    CanvasStruct
	(
		&ui_settingsPanelContainers,
		0,
		0,
		&g_ILI9320,
		160,
		132,
		20,
		20,
		CANVAS_STYLE_IMG,
		0,
		0,
		0,
		0,
		0,
		img_lightOff,
		0
	)
};

tCheckBoxWidget ui_settingsCheckBoxes[] =
{
	CheckBoxStruct
	(
		ui_settingsPanelContainers,
		ui_settingsCheckBoxes + 1,
		0,
		&g_ILI9320,
		10,
		40,
		110,
		45,
		CB_STYLE_FILL | CB_STYLE_TEXT,
		20,
		0,
		ClrSilver,
		ClrSilver,
		g_psFontCm16,
		"WIFI",
		0,
		onConnCheckBoxChange
	),

	CheckBoxStruct
	(
		ui_settingsPanelContainers,
		ui_settingsCheckBoxes + 2,
		0,
		&g_ILI9320,
		10,
		80,
		110,
		45,
		CB_STYLE_FILL | CB_STYLE_TEXT,
		20,
		0,
		ClrSilver,
		ClrSilver,
		g_psFontCm16,
		"Sensors",
		0,
		onConnCheckBoxChange
	),

	CheckBoxStruct
	(
		ui_settingsPanelContainers,
		ui_settingsCheckBoxIndicators,
		0,
		&g_ILI9320,
		10,
		120,
		120,
		45,
		CB_STYLE_FILL | CB_STYLE_TEXT,
		20,
		0,
		ClrSilver,
		ClrSilver,
		g_psFontCm16,
		"PowerSaving",
		0,
		onConnCheckBoxChange
	)
};

#define UI_SETTINGS_NUM_CONN_CHECKBOXES  (sizeof(ui_settingsCheckBoxes) / sizeof(ui_settingsCheckBoxes[0]))

/* Connect Button*/
RectangularButton
(
	ui_settingsConnectToApButton,
	ui_settingsPanelContainers+1,
	0,
	0,
	&g_ILI9320,
	200,
	52,
	100,
	28,
	PB_STYLE_FILL | PB_STYLE_TEXT | PB_STYLE_OUTLINE,
	ClrGreen,
	ClrRed,
	ClrSilver,
	ClrWhite,
	g_psFontCm12,
	"WIFI ON/OFF",
	0,
	0,
	0,
	0,
	onConnToAP
);


/* Setup Buttons*/
extern tPushButtonWidget ui_settingsSensorButton;
extern tPushButtonWidget ui_settingsOtherSettingsButton;
RectangularButton
(
	ui_settingsWifiSettingsButton,
	ui_settingsPanelContainers+2,
	&ui_settingsSensorButton,
	0,
	&g_ILI9320,
	20,
	185,
	80,
	35,
	PB_STYLE_FILL | PB_STYLE_TEXT | PB_STYLE_OUTLINE,
	ClrGreen,
	ClrRed,
	ClrSilver,
	ClrWhite,
	g_psFontCm12,
	"WIFI setup",
	0,
	0,
	0,
	0,
	onWifiSetup
);

RectangularButton
(
	ui_settingsSensorButton,
	ui_settingsPanelContainers+2,
	&ui_settingsOtherSettingsButton,
	0,
	&g_ILI9320,
	115,
	185,
	80,
	35,
	PB_STYLE_FILL | PB_STYLE_TEXT | PB_STYLE_OUTLINE,
	ClrGreen,
	ClrRed,
	ClrSilver,
	ClrWhite,
	g_psFontCm12,
	"Sensor setup",
	0,
	0,
	0,
	0,
	onSensorSetup
);

RectangularButton
(
	ui_settingsOtherSettingsButton,
	ui_settingsPanelContainers+2,
	0,
	0,
	&g_ILI9320,
	210,
	185,
	80,
	35,
	PB_STYLE_FILL | PB_STYLE_TEXT | PB_STYLE_OUTLINE,
	ClrGreen,
	ClrRed,
	ClrSilver,
	ClrWhite,
	g_psFontCm12,
	"Others setup",
	0,
	0,
	0,
	0,
	onOthersSetup
);

tContainerWidget ui_settingsPanelContainers[] =
{
	ContainerStruct
	(
		&ui_screenSettingsBackground,
		ui_settingsPanelContainers + 1,
		ui_settingsCheckBoxes,
		&g_ILI9320,
		8,
		24,
		180,
		148,
		CTR_STYLE_OUTLINE | CTR_STYLE_TEXT,
		0,
		ClrGray,
		ClrSilver,
		g_psFontCm16,
		"Connection Setup"
	),

	ContainerStruct
	(
		&ui_screenSettingsBackground,
		ui_settingsPanelContainers + 2,
		&ui_settingsConnectToApButton,
		&g_ILI9320,
		188,
		24,
		136-8-4,
		148,
		CTR_STYLE_OUTLINE | CTR_STYLE_TEXT,
		0,
		ClrGray,
		ClrSilver,
		g_psFontCm16,
		"Connect to AP"
	),

	ContainerStruct
	(
		&ui_screenSettingsBackground,
		0,
		&ui_settingsWifiSettingsButton,
		&g_ILI9320,
		8,
		173,
		320-8-4,
		55,
		CTR_STYLE_OUTLINE,
		0,
		ClrGray,
		ClrSilver,
		g_psFontCm12,
		NULL
	)
};

Canvas
(
	ui_screenSettingsBackground,
	WIDGET_ROOT,
	0,
	ui_settingsPanelContainers,
	&g_ILI9320,
	BG_MIN_X,
	BG_MIN_Y,
	BG_MAX_X - BG_MIN_X,
	BG_MAX_Y - BG_MIN_Y,
	CANVAS_STYLE_FILL,
	ClrBlack,
	0,
	0,
	0,
	0,
	0,
	0
);


void onConnCheckBoxChange(tWidget *widget, uint32_t enabled)
{
    uint32_t idx;

    for(idx = 0; idx < UI_SETTINGS_NUM_CONN_CHECKBOXES; ++idx)
    {
    	if (widget == &ui_settingsCheckBoxes[idx].sBase)
    	{
    		break;
    	}
    }
    //not found
    if (idx == UI_SETTINGS_NUM_CONN_CHECKBOXES)
    {
    	return;
    }
    CanvasImageSet(ui_settingsCheckBoxIndicators + idx,
    			   enabled ? img_lightOn : img_lightOff);
    WidgetPaint((tWidget *)(ui_settingsCheckBoxIndicators + idx));

    if (strcmp(ui_settingsCheckBoxes[idx].pcText, "WIFI") == 0)
    {
    	flashConfig->connectionSetupState.wifiEnabled = enabled;
    	configFlashSetModified(flashConfig);
    }
    else if (strcmp(ui_settingsCheckBoxes[idx].pcText, "Sensors") == 0)
    {
    	flashConfig->connectionSetupState.sensorsEnabled = enabled;
    	configFlashSetModified(flashConfig);
    }
    else if (strcmp(ui_settingsCheckBoxes[idx].pcText, "PowerSaving") == 0)
    {
    	flashConfig->connectionSetupState.powerSavingEnabled = enabled;
    	configFlashSetModified(flashConfig);
    }
}

void onConnToAP(tWidget *psWidget)
{
	if (!flashConfig->connectionSetupState.wifiEnabled)
	{
		return;
	}

	if (wifiGetConnectionStatus() == WIFI_NOT_CONNECTED)
	{
		if(wifiConnectToAp())
		{
			DEBUG(UI_SETTINGS_DEBUG_ENABLE, name, "Connected to AP: %s", eepromConfig->wifi_config.ap_ssid);
		}
		else
		{
			DEBUG(UI_SETTINGS_DEBUG_ENABLE, name, "Cannot connect to AP: %s",  eepromConfig->wifi_config.ap_ssid);
		}

	}
	else
	{
		if (wifiDisconnectFromAp())
		{
			DEBUG(UI_SETTINGS_DEBUG_ENABLE, name, "Disconnected from AP: %s",  eepromConfig->wifi_config.ap_ssid);
		}
		else
		{
			DEBUG(UI_SETTINGS_DEBUG_ENABLE, name, "ESP8266 not responding, check hardware!");
		}
	}
}


void onWifiSetup(tWidget *psWidget)
{
	if(flashConfig->connectionSetupState.wifiEnabled)
	{
        WidgetRemove(uiGetCurrentScreenContainer()->widget);
        uiSetCurrentScreen(SCREEN_WIFI_SETTINGS);
        WidgetAdd(WIDGET_ROOT, uiGetCurrentScreenContainer()->widget);
        WidgetPaint(WIDGET_ROOT);
	}
}

void onSensorSetup(tWidget *psWidget)
{
}

void onOthersSetup(tWidget *psWidget)
{
}


//*****************************************************************************
//
// The canvas widget acting as WIFI connection settings panel.
//
//*****************************************************************************

// The text entry button for the custom city.
RectangularButton
(
	ui_wifiApSsid,
	&ui_screenWifiSetupBackground,
	0,
	0,
	&g_ILI9320,
	118,
	30,
	190,
	28,
	PB_STYLE_FILL | PB_STYLE_TEXT,
	ClrLightGrey,
	ClrLightGrey,
	ClrWhite,
	ClrGray,
	g_psFontCmss14,
	ssidBuf,
	0,
	0,
	0,
	0,
	onSsidEntry
);


RectangularButton
(
	ui_wifiApPass,
	&ui_screenWifiSetupBackground,
	&ui_wifiApSsid,
	0,
	&g_ILI9320,
	118,
	70,
	190,
	28,
	PB_STYLE_FILL | PB_STYLE_TEXT,
	ClrLightGrey,
	ClrLightGrey,
	ClrWhite,
	ClrGray,
	g_psFontCmss14,
	passBuf,
	0,
	0,
	0,
	0,
	onPassEntry
);

RectangularButton
(
	ui_wifiCustomCity,
	&ui_screenWifiSetupBackground,
	&ui_wifiApPass,
	0,
	&g_ILI9320,
	118,
	110,
	190,
	28,
	PB_STYLE_FILL | PB_STYLE_TEXT,
	ClrLightGrey,
	ClrLightGrey,
	ClrWhite,
	ClrGray,
	g_psFontCmss14,
	cityBuf,
	0,
	0,
	0,
	0,
	onCityEntry
);

RectangularButton
(
	ui_wifiUpdateTime,
	&ui_screenWifiSetupBackground,
	&ui_wifiCustomCity,
	0,
	&g_ILI9320,
	118,
	150,
	190,
	28,
	PB_STYLE_FILL | PB_STYLE_TEXT,
	ClrLightGrey,
	ClrLightGrey,
	ClrWhite,
	ClrGray,
	g_psFontCmss14,
	0,
	0,
	0,
	0,
	0,
	onUpdateTimeEntry
);

/* the WIFIsettings panel. */
Canvas
(
	ui_screenWifiSetupBackground,
	WIDGET_ROOT,
	0,
	&ui_wifiUpdateTime,
	&g_ILI9320,
	BG_MIN_X,
	BG_MIN_Y,
	BG_MAX_X - BG_MIN_X,
	BG_MAX_Y - BG_MIN_Y,
	CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT_RIGHT | CANVAS_STYLE_TEXT_TOP,
	BG_COLOR_MAIN,
	ClrWhite,
	ClrBlueViolet,
	0,
	0,
	0,
	0
);


//*****************************************************************************
//
// // The canvas widget acting as Sensor settings panel.
//
//*****************************************************************************
/* the Sensor settings panel. */
//TODO
Canvas
(
	ui_screenSensorSetupBackground,
	WIDGET_ROOT,
	0,
	0,
	&g_ILI9320,
	BG_MIN_X,
	BG_MIN_Y,
	BG_MAX_X - BG_MIN_X,
	BG_MAX_Y - BG_MIN_Y,
	CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT_RIGHT | CANVAS_STYLE_TEXT_TOP,
	BG_COLOR_MAIN,
	ClrWhite,
	ClrBlueViolet,
	0,
	0,
	0,
	0
);



//*****************************************************************************
//
// Handles when the custom text area is pressed.
//
//*****************************************************************************
static void onCityEntry(tWidget *psWidget)
{
	// Disable swiping while the keyboard is active.
	//m_app_ctx.swipe_enabled = false;
	configEepromSetModified(eepromConfig);
	WidgetRemove(uiGetCurrentScreenContainer()->widget);
	uiKeyboardCreate(cityBuf, uiGetCurrentScreen(),
					AlphaNumeric, "Save the city", "Wanna save the city?",
					onParameterEdited);
	// Activate the keyboard.
	uiSetCurrentScreen(SCREEN_KEYBOARD);
}

static void onSsidEntry(tWidget *psWidget)
{
	configEepromSetModified(eepromConfig);
	WidgetRemove(uiGetCurrentScreenContainer()->widget);
	uiKeyboardCreate(ssidBuf, uiGetCurrentScreen(),
					AlphaNumeric, "Save the ap ssid", "Wanna save the AP SSID?",
					onParameterEdited);
	uiSetCurrentScreen(SCREEN_KEYBOARD);
}

static void onPassEntry(tWidget *psWidget)
{
	configEepromSetModified(eepromConfig);
	WidgetRemove(uiGetCurrentScreenContainer()->widget);
	uiKeyboardCreate(passBuf, uiGetCurrentScreen(),
					AlphaNumeric,"Save the AP pass", "Wanna save the AP password?",
					onParameterEdited);
	uiSetCurrentScreen(SCREEN_KEYBOARD);
}

static void onUpdateTimeEntry(tWidget *psWidget)
{
	configEepromSetModified(eepromConfig);
	WidgetRemove(uiGetCurrentScreenContainer()->widget);
	uiKeyboardCreate(timeEntryBuf, uiGetCurrentScreen(),
					Numeric, "Update refresh period", "Wanna save the period value?",
					onParameterEdited);
	uiSetCurrentScreen(SCREEN_KEYBOARD);
}

//*****************************************************************************
// @brief Callback which is passed to an active keyboard canvas to react on a parameter change (keyboard widget exit)
//*****************************************************************************
static void onParameterEdited(const Screens prevWidget, bool save)
{
	if (save)
	{
		memcpy(eepromConfig->city_name, cityBuf, sizeof(cityBuf));
		memcpy(eepromConfig->wifi_config.ap_wpa2_pass, passBuf, sizeof(passBuf));
		memcpy(eepromConfig->wifi_config.ap_ssid, ssidBuf, sizeof(ssidBuf));
		configFlashSaveSettingsToMemory(flashConfig);
		configEepromSaveSettingsToMemory(eepromConfig);
	}
	else
	{
		configFlashCheckAndCleanModified(flashConfig);
		configEepromCheckAndCleanModified(eepromConfig);
	}
	uiSetCurrentScreen(prevWidget);
    WidgetAdd(WIDGET_ROOT, uiGetCurrentScreenContainer()->widget);
	WidgetPaint(WIDGET_ROOT);
}

void uiScreenSettingsUpdate(void)
{
	if (uiGetCurrentScreen() != SCREEN_CONN_SETTINGS)
	{
		return;
	}
	tContext* drawingCtx = uiGetMainDrawingContext();

	if (flashConfig->connectionSetupState.wifiEnabled)
	{
		CheckBoxSelectedOn(&ui_settingsCheckBoxes[0]);
	    CanvasImageSet(&ui_settingsCheckBoxIndicators[0], img_lightOn);
	}
	else
	{
		CheckBoxSelectedOff(&ui_settingsCheckBoxes[0]);
	    CanvasImageSet(&ui_settingsCheckBoxIndicators[0], img_lightOff);
	}

	if (flashConfig->connectionSetupState.sensorsEnabled)
	{
		CheckBoxSelectedOn(&ui_settingsCheckBoxes[1]);
	    CanvasImageSet(&ui_settingsCheckBoxIndicators[1], img_lightOn);
	}
	else
	{
		CheckBoxSelectedOff(&ui_settingsCheckBoxes[1]);
	    CanvasImageSet(&ui_settingsCheckBoxIndicators[1], img_lightOff);
	}

	if (flashConfig->connectionSetupState.powerSavingEnabled)
	{
		CheckBoxSelectedOn(&ui_settingsCheckBoxes[2]);
	    CanvasImageSet(&ui_settingsCheckBoxIndicators[2], img_lightOn);
	}
	else
	{
		CheckBoxSelectedOff(&ui_settingsCheckBoxes[2]);
	    CanvasImageSet(&ui_settingsCheckBoxIndicators[2], img_lightOff);
	}
	for (size_t i = 0; i < 3; ++i)
	{
		WidgetPaint((tWidget *)(&ui_settingsCheckBoxes[i]));
		WidgetPaint((tWidget *)(&ui_settingsCheckBoxIndicators[i]));
	}

	//connection status
	switch (wifiGetConnectionStatus())
	{
		case WIFI_NOT_CONNECTED:
			GrContextFontSet(drawingCtx, &g_sFontCm12);
			GrContextForegroundSet(drawingCtx, ClrWhite);
			GrStringDrawCentered(drawingCtx, connStateDesc[WIFI_NOT_CONNECTED], -1, 250, 130, true);
			break;
		case WIFI_CONNECTED:
		case WIFI_TRANSMISSION_CREATED:
		case WIFI_TRANSMISSION_ENDED:
			GrContextFontSet(drawingCtx, &g_sFontCm16);
			GrContextForegroundSet(drawingCtx, ClrWhite);
			GrStringDrawCentered(drawingCtx, connStateDesc[WIFI_CONNECTED], -1, 250, 130, true);
			break;
		default:
			break;
	}
}

//
//@brief Stores NVM if sth changed
//
void uiScreenSettings_onExit(void)
{
	configFlashSaveSettingsToMemory(flashConfig);
	configEepromSaveSettingsToMemory(eepromConfig);
}
