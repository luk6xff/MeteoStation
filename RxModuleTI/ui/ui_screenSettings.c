/*
 * ui_screenSettings.c
 *
 *  Created on: 2 lip 2017
 *      Author: igbt6
 */
#include "ui_common.h"
#include "ui_keyboard.h"
#include "../images.h"


//*****************************************************************************
//
// Methods forward declarations.
//
//*****************************************************************************
static void onCityEntry(tWidget *psWidget);
static void onSsidEntry(tWidget *psWidget);
static void onPassEntry(tWidget *psWidget);
static void onUpdateTimeEntry(tWidget *psWidget);
static void onParameterEdited(const Screens prevWidget, bool save);

void onConnCheckBoxChange(tWidget *psWidget, uint32_t bSelected);
void onConnToAP(tWidget *psWidget);
void onWifiSetup(tWidget *psWidget);
void onSensorSetup(tWidget *psWidget);
void onOthersSetup(tWidget *psWidget);

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
		m_app_ctx.flash_params.connectionSetupState.wifiEnabled = enabled;
    }
    else if (strcmp(ui_settingsCheckBoxes[idx].pcText, "Sensors") == 0)
    {
		m_app_ctx.flash_params.connectionSetupState.sensorsEnabled = enabled;
    }
    else if (strcmp(ui_settingsCheckBoxes[idx].pcText, "PowerSaving") == 0)
    {
		m_app_ctx.flash_params.connectionSetupState.powerSavingEnabled = enabled;
    }
}

void onConnToAP(tWidget *psWidget)
{
	if (!m_app_ctx.flash_params.connectionSetupState.wifiEnabled  )
		return;

	if (wifiGetConnectionStatus() == WIFI_NOT_CONNECTED)
	{
		if(wifiConnectToAp())
		{
			MAIN_DEBUG("Connected to AP: %s", m_app_ctx.eeprom_params.wifi_config.ap_ssid);
		}
		else
		{
			MAIN_DEBUG("Cannot connect to AP: %s",  m_app_ctx.eeprom_params.wifi_config.ap_ssid);
		}

	}
	else if (wifiGetConnectionStatus())
	{
		if (wifiDisconnectFromAp())
		{
			MAIN_DEBUG("Disconnected from AP: %s",  m_app_ctx.eeprom_params.wifi_config.ap_ssid);
		}
		else
		{
			MAIN_DEBUG("ESP8266 not responding, check hardware!");
		}
	}
}


void onWifiSetup(tWidget *psWidget)
{
	MAIN_DEBUG("onWifiSetup pressed");
	if(m_app_ctx.flash_params.connectionSetupState.wifiEnabled)
	{
        WidgetRemove(m_screens[m_app_ctx.current_screen].widget);
        m_app_ctx.current_screen = SCREEN_WIFI_SETTINGS;
        WidgetAdd(WIDGET_ROOT, m_screens[m_app_ctx.current_screen].widget);
        WidgetPaint(WIDGET_ROOT);
	}
}

void onSensorSetup(tWidget *psWidget)
{
	MAIN_DEBUG("onSensorSetup pressed");
}

void onOthersSetup(tWidget *psWidget)
{
	MAIN_DEBUG("onOthersSetup pressed");
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
	m_app_ctx.eeprom_params.wifi_config.ap_ssid,
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
	m_app_ctx.eeprom_params.wifi_config.ap_wpa2_pass,
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
	m_app_ctx.eeprom_params.city_names[0],
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
// Widget acting as all settings panel.
//
//*****************************************************************************



//*****************************************************************************
//
// Handles when the custom text area is pressed.
//
//*****************************************************************************
static void onCityEntry(tWidget *psWidget)
{
	// Disable swiping while the keyboard is active.
	m_app_ctx.swipe_enabled = false;
	WidgetRemove(m_screens[m_app_ctx.current_screen].widget);
	//configEepromSetModified(&m_app_ctx.eeprom_params); // param in eeprom will be modified
	uiKeyboardCreate(m_app_ctx.eeprom_params.city_names[m_app_ctx.flash_params.currentCity], m_app_ctx.current_screen,
					AlphaNumeric, "Save the city", "Wanna save the city?",
					onParameterEdited);
	// Activate the keyboard.
	uiSetCurrentScreen(SCREEN_KEYBOARD);
}

static void onSsidEntry(tWidget *psWidget)
{
	m_app_ctx.swipe_enabled = false;
	WidgetRemove(m_screens[m_app_ctx.current_screen].widget);
	configEepromSetModified(&m_app_ctx.eeprom_params); // param in eeprom will be modified
	uiKeyboardCreate(m_app_ctx.eeprom_params.wifi_config.ap_ssid, m_app_ctx.current_screen,
					AlphaNumeric, "Save the ap ssid", "Wanna save the AP SSID?",
					onParameterEdited);
	uiSetCurrentScreen(SCREEN_KEYBOARD);
}

static void onPassEntry(tWidget *psWidget)
{
	m_app_ctx.swipe_enabled = false;
	WidgetRemove(m_screens[m_app_ctx.current_screen].widget);
	configEepromSetModified(&m_app_ctx.eeprom_params); // param in eeprom will be modified
	uiKeyboardCreate(m_app_ctx.eeprom_params.wifi_config.ap_wpa2_pass, m_app_ctx.current_screen,
					AlphaNumeric,"Save the AP pass", "Wanna save the AP password?",
					onParameterEdited);
	uiSetCurrentScreen(SCREEN_KEYBOARD);
}

static void onUpdateTimeEntry(tWidget *psWidget)
{
	m_app_ctx.swipe_enabled = false;
	WidgetRemove(m_screens[m_app_ctx.current_screen].widget);
	configEepromSetModified(&m_app_ctx.eeprom_params); // param in eeprom will be modified
	uiKeyboardCreate(m_app_ctx.eeprom_params.update_wifi_period_time, m_app_ctx.current_screen,
					Numeric, "Update refresh period", "Wanna save the period value?",
					onParameterEdited);
	uiSetCurrentScreen(SCREEN_KEYBOARD);
}

//*****************************************************************************
// @brief Callback which is passed to an active keyboard canvas to react on a parameter change (keyboard widget exit)
//*****************************************************************************
static void onParameterEdited(const Screens prevWidget, bool save)
{
	if(save)
	{
		saveApplicationContextToMemory();
	}
	else
	{
		configFlashCheckAndCleanModified(&m_app_ctx.flash_params);
		configEepromCheckAndCleanModified(&m_app_ctx.eeprom_params);
	}
	m_app_ctx.current_screen = prevWidget;
    WidgetAdd(WIDGET_ROOT, m_screens[m_app_ctx.current_screen].widget);
	WidgetPaint(WIDGET_ROOT);
    //Enable swiping after removing keyboard from the root
    m_app_ctx.swipe_enabled = true;
}
