#include <string.h>

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"

#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/systick.h"
#include "driverlib/interrupt.h"
#include "driverlib/fpu.h"

#include "utils/ustdlib.h"

#include "delay.h"
#include "3PointCalibration.h"
#include "config.h"
#include "wifi.h"
#include "ili9320_driver.h"
#include "touch.h"
#include "images.h"
#include "system.h"
#include "ui/ui_message_box.h"
#include "ui/ui_keyboard.h"
#include "ui/ui_common.h"

#define MAIN_DEBUG_ENABLE 1
#if MAIN_DEBUG_ENABLE
	#include "debugConsole.h"
#endif
static inline void MAIN_DEBUG(const char* fmt, ...)
{
	do
	{
		if (MAIN_DEBUG_ENABLE)
		{
			va_list args;
			va_start(args, fmt);
			debugConsolePrintf("Main: ");
			debugConsolePrintf(fmt, args);
			va_end(args);
			debugConsolePrintf("\n");
		}
	}while(0);
}








//*****************************************************************************
//
// Main State machine states
//
//*****************************************************************************
typedef enum
{
	STATE_RESET,
	STATE_INIT,
	STATE_CHECK_CONNECTION,
	STATE_GET_WEATHER_FROM_WEB,
	STATE_GET_WEATHER_FROM_SENSOR,
	STATE_UPDATE_DISPLAY
}State;

//*****************************************************************************
//
// Connection  states
//
//*****************************************************************************


typedef enum
{
	SENSOR_NOT_CONNECTED,
	SENSOR_CONNECTED,
	SENSOR_WAIT_FOR_DATA,
}SensorConnectionState;

static const char* const connStateDesc[] = {
		"NOT_CONNECTED",
		"CONNECTED",
		"WAIT_FOR_DATA",
		"Connect",
		"Disconnect"
};

typedef struct
{
	tWidget *widget;
	Screens up;
	Screens down;
	Screens left;
	Screens right;
}ScreenContainer;

//*****************************************************************************
//
// Forward reference to all used widget structures.
//
//*****************************************************************************
extern tCanvasWidget g_screenMainBackground;
extern tCanvasWidget g_screenWifiSetupBackground;
extern tCanvasWidget g_screenSensorSetupBackground;
extern tCanvasWidget g_settingsPanel;


//*****************************************************************************
//
// Typedefs.
//
//*****************************************************************************
//*****************************************************************************
//
// Structure that describes swipes data.
//
//*****************************************************************************
typedef struct
{
	#define  MIN_SWIPE_DIFFERENCE 60
	#define  LAST_VAL_BUF_SIZE 10
	int32_t initX;
	int32_t initY;
	int32_t bufX[LAST_VAL_BUF_SIZE];
	int32_t bufY[LAST_VAL_BUF_SIZE];
	uint8_t sampleNum;
	bool swipeStarted;
	bool swipeOnGoing;
    enum
    {
        SWIPE_UP,
		SWIPE_DOWN,
		SWIPE_LEFT,
		SWIPE_RIGHT,
		SWIPE_NONE,
    }
    swipeDirecttion;
}Swipe;


//*****************************************************************************
//
// Structure that describes whole Application Context
//
//*****************************************************************************
typedef struct
{
	ConfigFlashParameters flash_params;
	ConfigEepromParameters eeprom_params;
	WifiConnectionState wifi_state;
	SensorConnectionState sensor_state;
	Screens current_screen;
	bool swipe_enabled;

}AppContext;

//*****************************************************************************
//
// Application globals
//
//*****************************************************************************

static volatile State g_mainState = STATE_RESET;
static volatile Swipe m_swipe;
static tContext m_drawing_context;
static AppContext m_app_ctx;

static ScreenContainer m_screens[SCREEN_NUM_OF_SCREENS] =
{
    {
        (tWidget *)&g_screenMainBackground,
        SCREEN_MAIN, SCREEN_CONN_SETTINGS, SCREEN_MAIN, SCREEN_MAIN
    },
    {
        (tWidget *)&g_settingsPanel,
        SCREEN_MAIN, SCREEN_CONN_SETTINGS, SCREEN_CONN_SETTINGS, SCREEN_CONN_SETTINGS
    },
    {
        (tWidget *)&g_screenWifiSetupBackground,
		SCREEN_CONN_SETTINGS, SCREEN_CONN_SETTINGS, SCREEN_CONN_SETTINGS, SCREEN_CONN_SETTINGS
    },
    {
        (tWidget *)&g_screenSensorSetupBackground,
		SCREEN_CONN_SETTINGS, SCREEN_CONN_SETTINGS, SCREEN_CONN_SETTINGS, SCREEN_CONN_SETTINGS
    }
};



//*****************************************************************************
//
// Methods forward declarations.
//
//*****************************************************************************
static void onWifiEnable(tWidget *psWidget);
static void onCityEntry(tWidget *psWidget);
static void onSsidEntry(tWidget *psWidget);
static void onPassEntry(tWidget *psWidget);
static void onUpdateTimeEntry(tWidget *psWidget);
static void onParameterEdited(const Screens prevWidget, bool save);
static int32_t touchScreenCallback(uint32_t msg, int32_t x, int32_t y);

//*****************************************************************************
//
// Setters for App Context params
//
//*****************************************************************************
static void updateWifiConnectionStatus(WifiConnectionState state);
static void updateSensorConnectionStatus(SensorConnectionState state);

static bool saveApplicationContextToMemory()
{
	bool ret = false;
	if(configFlashCheckAndCleanModified(&m_app_ctx.flash_params))
	{
		memcpy(configFlashGetCurrent(), &m_app_ctx.flash_params, sizeof(ConfigFlashParameters));
		configFlashSave();
		ret = true;
	}

	if(configEepromCheckAndCleanModified(&m_app_ctx.eeprom_params))
	{
		memcpy(configEepromGetCurrent(), &m_app_ctx.eeprom_params, sizeof(ConfigEepromParameters));
		configEepromSave();
		ret = true;
	}
	return ret;
}

// Reads config from flash and eeprom and fills the current App context
static bool readConfigAndSetAppContext()
{
	// assign defaults
	memcpy(&m_app_ctx.flash_params, configFlashGetDefaultSettings(), sizeof(ConfigFlashParameters));
	memcpy(&m_app_ctx.eeprom_params, configEepromGetDefaultSettings(), sizeof(ConfigEepromParameters));

	// reads configuration from flash/eeprom
	configInit();

	if(configFlashIsInvalid(configFlashGetCurrent()))
	{
		configFlashSaveDefaults();
	}
	else
	{
		// assign defaults settings from memory
		memcpy(&m_app_ctx.flash_params, configFlashGetCurrent(), sizeof(ConfigFlashParameters));
	}

	if(configEepromIsInvalid(configEepromGetCurrent()))
	{
		configEepromSaveDefaults();
	}
	else
	{
		// assign defaults settings from memory
		memcpy(&m_app_ctx.eeprom_params, configEepromGetCurrent(), sizeof(ConfigEepromParameters));
	}

	m_app_ctx.current_screen = SCREEN_MAIN;
	m_app_ctx.swipe_enabled = true;

	return true;
}



//*****************************************************************************
//
// Touch screen calibration method.
//
//*****************************************************************************
static bool setTouchScreenCalibration()
{
	CalibCoefficients coeffs;
	bool ret = false;
	bool intsOff;
	//disable all interrupts
	intsOff = IntMasterDisable();
	ConfigEepromParameters* cfg = &m_app_ctx.eeprom_params;
	if(configEepromIsInvalid(cfg) || cfg->touch_screen_params.is_valid == 0x00 || // first time enabled on the plant
	   configEepromCheckAndCleanModified(cfg)) // if you want manually run the calibration
	{
		performThreePointCalibration(&m_drawing_context, &coeffs);
		ADS7843setCalibrationCoefficients(&coeffs);
		if(confirmThreePointCalibration(&m_drawing_context))
		{
			cfg->touch_screen_params.calib_coeffs = coeffs;
			cfg->touch_screen_params.is_valid = 0x01;
			cfg->params_version = 0x01;
			configEepromSetModified(cfg);
			saveApplicationContextToMemory();
			MAIN_DEBUG("COEFFSa: a.x=%d, a.y=%d\n\r", coeffs.m_ax, coeffs.m_ay);
			MAIN_DEBUG("COEFFSb: b.x=%d, b.y=%d\n\r", coeffs.m_bx, coeffs.m_by);
			MAIN_DEBUG("COEFFSd: d.x=%d, d.y=%d\n\r", coeffs.m_dx, coeffs.m_dy);
		}
		else
		{
			MAIN_DEBUG("Touch Screen Calibration failed\n\r");
		}

		ret = true;
	}
	else
	{
		ADS7843setCalibrationCoefficients(&cfg->touch_screen_params.calib_coeffs);
	}


	if(!intsOff)
	{
		ENABLE_ALL_INTERRUPTS();
	}
	return ret;
}


// Main page widgets
char g_pcTempHighLow[40]="--/--C";
Canvas(g_sTempHighLow, &g_screenMainBackground, 0, 0,
       &g_ILI9320, 120, 195, 70, 30,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_LEFT |
       CANVAS_STYLE_TEXT_TOP | CANVAS_STYLE_TEXT_OPAQUE, BG_COLOR_MAIN,
       ClrWhite, ClrWhite, g_psFontCmss20, g_pcTempHighLow, 0, 0);

char g_pcTemp[40]="--C";
Canvas(g_sTemp, &g_screenMainBackground, &g_sTempHighLow, 0,
       &g_ILI9320, 20, 175, 100, 50,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_RIGHT |
       CANVAS_STYLE_TEXT_TOP | CANVAS_STYLE_TEXT_OPAQUE, BG_COLOR_MAIN,
       ClrWhite, ClrWhite, g_psFontCmss48, g_pcTemp, 0, 0);

char g_pcHumidity[40]="Cisnienie: ----hPa";
Canvas(g_sHumidity, &g_screenMainBackground, &g_sTemp, 0,
       &g_ILI9320, 20, 140, 160, 25,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_LEFT |
       CANVAS_STYLE_TEXT_OPAQUE, BG_COLOR_MAIN, ClrWhite, ClrWhite,
       g_psFontCmss20, g_pcHumidity, 0, 0);

char g_pcStatus[40];
Canvas(g_sStatus, &g_screenMainBackground, &g_sHumidity, 0,
       &g_ILI9320, 20, 110, 160, 25,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_LEFT |
       CANVAS_STYLE_TEXT_OPAQUE, BG_COLOR_MAIN, ClrWhite, ClrWhite,
       g_psFontCmss20, g_pcStatus, 0, 0);

char g_pcCity[40];
Canvas(g_sCityName, &g_screenMainBackground, &g_sStatus, 0,
       &g_ILI9320, 20, 40, 240, 25,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_LEFT |
       CANVAS_STYLE_TEXT_OPAQUE, BG_COLOR_MAIN, ClrWhite, ClrWhite,
       g_psFontCmss20, g_pcCity, 0, 0);

Canvas(g_screenMainBackground, WIDGET_ROOT, 0, &g_sCityName,
       &g_ILI9320, BG_MIN_X, BG_MIN_Y,
       BG_MAX_X - BG_MIN_X,
       BG_MAX_Y - BG_MIN_Y, CANVAS_STYLE_FILL,
       BG_COLOR_MAIN, ClrWhite, ClrBlueViolet, g_psFontCmss20,
       0, 0, 0);

//*****************************************************************************
//
// The canvas widget acting as WIFI connection settings panel.
//
//*****************************************************************************

// The text entry button for the custom city.
RectangularButton(g_wifiApSsid, &g_screenWifiSetupBackground, 0, 0,
       &g_ILI9320, 118, 30, 190, 28,
       PB_STYLE_FILL | PB_STYLE_TEXT, ClrLightGrey,
       ClrLightGrey, ClrWhite, ClrGray, g_psFontCmss14,
	   m_app_ctx.eeprom_params.wifi_config[0].ap_ssid, 0, 0, 0 ,0 , onSsidEntry);

RectangularButton(g_wifiApPass, &g_screenWifiSetupBackground, &g_wifiApSsid, 0,
       &g_ILI9320, 118, 70, 190, 28,
       PB_STYLE_FILL | PB_STYLE_TEXT, ClrLightGrey,
       ClrLightGrey, ClrWhite, ClrGray, g_psFontCmss14,
	   m_app_ctx.eeprom_params.wifi_config[0].ap_wpa2_pass, 0, 0, 0 ,0 , onPassEntry);

RectangularButton(g_wifiCustomCity, &g_screenWifiSetupBackground, &g_wifiApPass, 0,
       &g_ILI9320, 118, 110, 190, 28,
       PB_STYLE_FILL | PB_STYLE_TEXT, ClrLightGrey,
       ClrLightGrey, ClrWhite, ClrGray, g_psFontCmss14,
	   m_app_ctx.eeprom_params.city_names[0], 0, 0, 0 ,0 , onCityEntry);

RectangularButton(g_wifiUpdateTime, &g_screenWifiSetupBackground, &g_wifiCustomCity, 0,
       &g_ILI9320, 118, 150, 190, 28,
       PB_STYLE_FILL | PB_STYLE_TEXT, ClrLightGrey,
       ClrLightGrey, ClrWhite, ClrGray, g_psFontCmss14,
       0, 0, 0, 0 ,0 , onUpdateTimeEntry);

/* the WIFIsettings panel. */
Canvas(g_screenWifiSetupBackground, WIDGET_ROOT, 0, &g_wifiUpdateTime/*&g_screenWifiSetupIPAddr*/,
       &g_ILI9320, BG_MIN_X, BG_MIN_Y,
       BG_MAX_X - BG_MIN_X, BG_MAX_Y - BG_MIN_Y,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT_RIGHT |
       CANVAS_STYLE_TEXT_TOP, BG_COLOR_MAIN, ClrWhite, ClrBlueViolet, 0,
       0, 0, 0);

//*****************************************************************************
//
// // The canvas widget acting as Sensor settings panel.
//
//*****************************************************************************
/* the Sensor settings panel. */
//TODO
Canvas(g_screenSensorSetupBackground, WIDGET_ROOT, 0, 0,
       &g_ILI9320, BG_MIN_X, BG_MIN_Y,
       BG_MAX_X - BG_MIN_X, BG_MAX_Y - BG_MIN_Y,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT_RIGHT |
       CANVAS_STYLE_TEXT_TOP, BG_COLOR_MAIN, ClrWhite, ClrBlueViolet, 0,
       0, 0, 0);


//*****************************************************************************
//
// Widget acting as all settings panel.
//
//*****************************************************************************
void onConnCheckBoxChange(tWidget *psWidget, uint32_t bSelected);
void onConnToAP(tWidget *psWidget);
void onWifiSetup(tWidget *psWidget);
void onSensorSetup(tWidget *psWidget);
void onOthersSetup(tWidget *psWidget);

tContainerWidget g_panelConnContainers[];
tPushButtonWidget g_connTestButton;
tCanvasWidget g_connCheckBoxIndicators[] =
{
    CanvasStruct(&g_panelConnContainers, g_connCheckBoxIndicators + 1, 0,
                 &g_ILI9320, 160, 52, 20, 20,
                 CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pui8LightOff, 0),
    CanvasStruct(&g_panelConnContainers, g_connCheckBoxIndicators + 2, 0,
                 &g_ILI9320, 160, 92, 20, 20,
                 CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pui8LightOff, 0),
    CanvasStruct(&g_panelConnContainers, 0, 0,
                 &g_ILI9320, 160, 132, 20, 20,
                 CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pui8LightOff, 0),
};
tCheckBoxWidget g_connCheckBoxes[] =
{
		CheckBoxStruct(g_panelConnContainers, g_connCheckBoxes + 1, 0,
                      &g_ILI9320, 10, 40, 110, 45,
					  CB_STYLE_FILL | CB_STYLE_TEXT, 20,
					  0, ClrSilver, ClrSilver, g_psFontCm16,
                      "WIFI", 0, onConnCheckBoxChange),
		CheckBoxStruct(g_panelConnContainers, g_connCheckBoxes + 2, 0,
                      &g_ILI9320, 10, 80, 110, 45,
					  CB_STYLE_FILL | CB_STYLE_TEXT, 20,
					  0, ClrSilver, ClrSilver, g_psFontCm16,
                      "Sensors", 0, onConnCheckBoxChange),
		CheckBoxStruct(g_panelConnContainers, g_connCheckBoxIndicators, 0,
                      &g_ILI9320, 10, 120, 120, 45,
					  CB_STYLE_FILL | CB_STYLE_TEXT, 20,
					  0, ClrSilver, ClrSilver, g_psFontCm16,
                      "PowerSaving", 0, onConnCheckBoxChange)
};

#define NUM_CONN_CHECKBOXES  (sizeof(g_connCheckBoxes) / sizeof(g_connCheckBoxes[0]))

/* Connect Button*/
RectangularButton(g_connToApButton, g_panelConnContainers+1, 0, 0,
				  &g_ILI9320, 200, 52, 100, 28,
				  PB_STYLE_FILL | PB_STYLE_TEXT | PB_STYLE_OUTLINE,
				  ClrGreen, ClrRed, ClrSilver, ClrWhite, g_psFontCmss14,
				  "State", 0, 0, 0 ,0 , onConnToAP);


/* Setup Buttons*/
extern tPushButtonWidget g_sensorSettingsButton;
extern tPushButtonWidget g_othersSettingsButton;
RectangularButton(g_wifiSettingsButton, g_panelConnContainers+2, &g_sensorSettingsButton, 0,
				  &g_ILI9320, 20, 185, 80, 35,
				  PB_STYLE_FILL | PB_STYLE_TEXT | PB_STYLE_OUTLINE,
				  ClrGreen, ClrRed, ClrSilver, ClrWhite, g_psFontCm12,
				  "WIFI setup", 0, 0, 0 ,0 , onWifiSetup);

RectangularButton(g_sensorSettingsButton, g_panelConnContainers+2, &g_othersSettingsButton, 0,
				  &g_ILI9320, 115, 185, 80, 35,
				  PB_STYLE_FILL | PB_STYLE_TEXT | PB_STYLE_OUTLINE,
				  ClrGreen, ClrRed, ClrSilver, ClrWhite, g_psFontCm12,
				  "Sensor setup", 0, 0, 0 ,0 , onSensorSetup);

RectangularButton(g_othersSettingsButton, g_panelConnContainers+2, 0, 0,
				  &g_ILI9320, 210, 185, 80, 35,
				  PB_STYLE_FILL | PB_STYLE_TEXT | PB_STYLE_OUTLINE,
				  ClrGreen, ClrRed, ClrSilver, ClrWhite, g_psFontCm12,
				  "Others setup", 0, 0, 0 ,0 , onOthersSetup);

tContainerWidget g_panelConnContainers[] = {
		ContainerStruct(&g_settingsPanel, g_panelConnContainers + 1, g_connCheckBoxes,
						&g_ILI9320, 8, 24, 180, 148,
						CTR_STYLE_OUTLINE | CTR_STYLE_TEXT, 0, ClrGray, ClrSilver,
						g_psFontCm16, "Connection Setup"),
		ContainerStruct(&g_settingsPanel, g_panelConnContainers + 2, &g_connToApButton,
						&g_ILI9320, 188, 24, 136-8-4, 148,
						CTR_STYLE_OUTLINE | CTR_STYLE_TEXT, 0, ClrGray, ClrSilver,
						g_psFontCm16, "Connect to AP"),
		ContainerStruct(&g_settingsPanel, 0, &g_wifiSettingsButton,
						&g_ILI9320, 8, 173, 320-8-4, 55,
						CTR_STYLE_OUTLINE, 0, ClrGray, ClrSilver,
						g_psFontCm12, NULL),
};

Canvas(g_settingsPanel, WIDGET_ROOT, 0, g_panelConnContainers,
       &g_ILI9320, BG_MIN_X, BG_MIN_Y,
       BG_MAX_X - BG_MIN_X,
       BG_MAX_Y - BG_MIN_Y, CANVAS_STYLE_FILL,
	   ClrBlack, 0, 0, 0, 0, 0, 0);

void onConnCheckBoxChange(tWidget *widget, uint32_t enabled)
{
    uint32_t idx;

    for(idx = 0; idx < NUM_CONN_CHECKBOXES; ++idx)
    {
    	if(widget == &g_connCheckBoxes[idx].sBase)
    	{
    		break;
    	}
    }
    //not found
    if(idx == NUM_CONN_CHECKBOXES)
    {
    	return;
    }
    CanvasImageSet(g_connCheckBoxIndicators + idx,
    			   enabled ? g_pui8LightOn : g_pui8LightOff);
    WidgetPaint((tWidget *)(g_connCheckBoxIndicators + idx));
    if(strcmp(g_connCheckBoxes[idx].pcText, "WIFI") == 0)
    {
    		m_app_ctx.flash_params.connectionSetupState.wifiEnabled = enabled;
    }
    else if(strcmp(g_connCheckBoxes[idx].pcText, "Sensors") == 0)
    {
    		m_app_ctx.flash_params.connectionSetupState.sensorsEnabled = enabled;
    }
    else if(strcmp(g_connCheckBoxes[idx].pcText, "PowerSaving") == 0)
    {
    		m_app_ctx.flash_params.connectionSetupState.powerSavingEnabled = enabled;
    }
}

void onConnToAP(tWidget *psWidget)
{
	if(!m_app_ctx.flash_params.connectionSetupState.wifiEnabled  )
		return;
	WifiConnectionState state = m_app_ctx.wifi_state;
	if(state == WIFI_NOT_CONNECTED)
	{
		if(!esp8266CommandAT())
		{
			MAIN_DEBUG("ESP8266 not responding, check hardware!");
		}
		else
		{
			if(esp8266CommandCWJAP(m_app_ctx.eeprom_params.wifi_config[0].ap_ssid, m_app_ctx.eeprom_params.wifi_config[0].ap_wpa2_pass))
			{
				MAIN_DEBUG("Connected to AP: %s", m_app_ctx.eeprom_params.wifi_config[0].ap_ssid);
				state = WIFI_CONNECTED;
			}
			else
			{
				MAIN_DEBUG("Cannot connect to AP: %s",  m_app_ctx.eeprom_params.wifi_config[0].ap_ssid);
			}
		}
	}
	else if(state == WIFI_CONNECTED)
	{
		if(esp8266CommandCWQAP())
		{
			MAIN_DEBUG("Disconnected from AP: %s",  m_app_ctx.eeprom_params.wifi_config[0].ap_ssid);
			state = WIFI_NOT_CONNECTED;
		}
		else
		{
			MAIN_DEBUG("ESP8266 not responding, check hardware!");
		}
	}
	updateWifiConnectionStatus(state);
}


//TODO
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

static void updateWifiConnectionStatus(WifiConnectionState state)
{
	//if(state == m_app_ctx.wifi_state)
	//{
	//	return;
	//}
	switch (state) {
		case WIFI_NOT_CONNECTED:
			if(m_app_ctx.current_screen == SCREEN_CONN_SETTINGS)
			{
			    GrContextFontSet(&m_drawing_context, &g_sFontCm12);
			    GrContextForegroundSet(&m_drawing_context, ClrWhite);
			    GrStringDrawCentered(&m_drawing_context, connStateDesc[WIFI_NOT_CONNECTED], -1, 250, 130, true);
			    g_connToApButton.pcText = connStateDesc[WIFI_WAIT_FOR_DATA+1];
			}
			break;
		case WIFI_CONNECTED:
			if(m_app_ctx.current_screen == SCREEN_CONN_SETTINGS)
			{
			    GrContextFontSet(&m_drawing_context, &g_sFontCm16);
			    GrContextForegroundSet(&m_drawing_context, ClrWhite);
			    GrStringDrawCentered(&m_drawing_context, connStateDesc[WIFI_CONNECTED], -1, 250, 130, true);
			    g_connToApButton.pcText = connStateDesc[WIFI_WAIT_FOR_DATA+2];
			}
			break;
		case WIFI_WAIT_FOR_DATA:
			if(m_app_ctx.current_screen == SCREEN_CONN_SETTINGS)
			{
			    GrContextFontSet(&m_drawing_context, &g_sFontCm16);
			    GrContextForegroundSet(&m_drawing_context, ClrWhite);
			    GrStringDrawCentered(&m_drawing_context, connStateDesc[WIFI_WAIT_FOR_DATA], -1, 250, 130, true);
			}
			break;
		default:
			break;
	}
	m_app_ctx.flash_params.connectionSetupState.wifiConnectionState = state;
}

static void updateSensorConnectionStatus(SensorConnectionState state)
{

}




static void onWifiEnable(tWidget *psWidget)

{
	if(m_app_ctx.flash_params.connectionSetupState.wifiEnabled)
	{
		m_app_ctx.flash_params.connectionSetupState.wifiEnabled = false;
		PushButtonTextColorSet(&g_wifiCustomCity, ClrGray);
	}
	else
	{
		m_app_ctx.flash_params.connectionSetupState.wifiEnabled  = true;
		PushButtonTextColorSet(&g_wifiCustomCity, ClrBlack);
	}
}


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
	configEepromSetModified(&m_app_ctx.eeprom_params); // param in eeprom will be modified
	uiKeyboardCreate(m_app_ctx.eeprom_params.city_names[0], m_app_ctx.current_screen,
					AlphaNumeric, "Save the city", "Wanna save the city?",
					onParameterEdited);
	// Activate the keyboard.
	m_app_ctx.current_screen = SCREEN_KEYBOARD;
}

static void onSsidEntry(tWidget *psWidget)
{
	m_app_ctx.swipe_enabled = false;
	WidgetRemove(m_screens[m_app_ctx.current_screen].widget);
	configEepromSetModified(&m_app_ctx.eeprom_params); // param in eeprom will be modified
	uiKeyboardCreate(m_app_ctx.eeprom_params.wifi_config[0].ap_ssid, m_app_ctx.current_screen,
					AlphaNumeric, "Save the ap ssid", "Wanna save the AP SSID?",
					onParameterEdited);
	m_app_ctx.current_screen = SCREEN_KEYBOARD;
}

static void onPassEntry(tWidget *psWidget)
{
	m_app_ctx.swipe_enabled = false;
	WidgetRemove(m_screens[m_app_ctx.current_screen].widget);
	configEepromSetModified(&m_app_ctx.eeprom_params); // param in eeprom will be modified
	uiKeyboardCreate(m_app_ctx.eeprom_params.wifi_config[0].ap_wpa2_pass, m_app_ctx.current_screen,
					AlphaNumeric,"Save the AP pass", "Wanna save the AP password?",
					onParameterEdited);
	m_app_ctx.current_screen = SCREEN_KEYBOARD;
}

static void onUpdateTimeEntry(tWidget *psWidget)
{
	m_app_ctx.swipe_enabled = false;
	WidgetRemove(m_screens[m_app_ctx.current_screen].widget);
	configEepromSetModified(&m_app_ctx.eeprom_params); // param in eeprom will be modified
	uiKeyboardCreate(m_app_ctx.eeprom_params.update_wifi_period_time, m_app_ctx.current_screen,
					Numeric, "Update refresh period", "Wanna save the period value?",
					onParameterEdited);
	m_app_ctx.current_screen = SCREEN_KEYBOARD;
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


//*****************************************************************************
//
// The callback function that is called by the touch screen driver to indicate
// activity on the touch screen.
//
//*****************************************************************************
static int32_t touchScreenCallback(uint32_t msg, int32_t x, int32_t y)
{
	int32_t swipeDiffX, swipeDiffY;
    if(m_app_ctx.swipe_enabled)
    {
        switch(msg)
        {
            // The user has just touched the screen.
            case WIDGET_MSG_PTR_DOWN:
            {
                // Save this press location.
            	m_swipe.swipeStarted = true;
            	if(!m_swipe.swipeOnGoing)
            	{
					m_swipe.initX = x;
					m_swipe.initY = y;

            	}
            	else
            	{	++m_swipe.sampleNum;
            		m_swipe.bufX[m_swipe.sampleNum % LAST_VAL_BUF_SIZE] = x;
            		m_swipe.bufY[m_swipe.sampleNum % LAST_VAL_BUF_SIZE] = y;
            	}

                break;
            }

            // The user has moved the touch location on the screen.
            case WIDGET_MSG_PTR_MOVE:
            {
            	if(m_swipe.swipeStarted || m_swipe.swipeOnGoing)
            	{
            		++m_swipe.sampleNum;
            		m_swipe.bufX[m_swipe.sampleNum % LAST_VAL_BUF_SIZE] = x;
            		m_swipe.bufY[m_swipe.sampleNum % LAST_VAL_BUF_SIZE] = y;
					m_swipe.swipeOnGoing = true;
            	}
                break;
            }

            // The user just stopped touching the screen.
            case WIDGET_MSG_PTR_UP:
            {
            	if(m_swipe.swipeOnGoing)
            	{
            		//checks on last gathered data for now
            		int32_t xLastVal = m_swipe.bufX[m_swipe.sampleNum % LAST_VAL_BUF_SIZE];
            		int32_t yLastVal = m_swipe.bufY[m_swipe.sampleNum % LAST_VAL_BUF_SIZE];
            		bool xLessThanInit = xLastVal < m_swipe.initX;
            		bool yLessThanInit = yLastVal < m_swipe.initY;
            		swipeDiffX = ((xLastVal - m_swipe.initX)>0) ? (xLastVal - m_swipe.initX) : (m_swipe.initX - xLastVal);
            		swipeDiffY = ((yLastVal - m_swipe.initY)>0) ? (yLastVal - m_swipe.initY) : (m_swipe.initY - yLastVal);
            		// checks which difference is bigger
            		if(swipeDiffX > swipeDiffY )
            		{

            			if(!xLessThanInit && (swipeDiffX > MIN_SWIPE_DIFFERENCE))
						{
							m_swipe.swipeDirecttion = SWIPE_RIGHT;
						}
						else if(xLessThanInit && (swipeDiffX > MIN_SWIPE_DIFFERENCE))
						{
							m_swipe.swipeDirecttion = SWIPE_LEFT;
						}
            		}
            		else
            		{
						if(!yLessThanInit && (swipeDiffY > MIN_SWIPE_DIFFERENCE))
						{
							m_swipe.swipeDirecttion = SWIPE_DOWN;
						}
						else if(yLessThanInit && (swipeDiffY > MIN_SWIPE_DIFFERENCE))
						{
							m_swipe.swipeDirecttion = SWIPE_UP;
						}
            		}
            	}
        		m_swipe.swipeOnGoing = false;
        		m_swipe.swipeStarted = false;
        		m_swipe.sampleNum = 0;
        		break;
            }
            default:
        		m_swipe.swipeOnGoing = false;
        		m_swipe.swipeStarted = false;
        		m_swipe.sampleNum = 0;
        		break;
        }
    }
    WidgetPointerMessage(msg, x, y);
    return(0);
}


static void handleMovement(void)
{
	uint16_t newScreenIdx = m_app_ctx.current_screen;
	if(m_app_ctx.swipe_enabled)
	{
		if(m_swipe.swipeDirecttion != SWIPE_NONE )
		{

			if(m_swipe.swipeDirecttion == SWIPE_RIGHT)
			{
			    newScreenIdx = m_screens[m_app_ctx.current_screen].right;
			}
			else if(m_swipe.swipeDirecttion == SWIPE_LEFT)
			{
			    newScreenIdx = m_screens[m_app_ctx.current_screen].left;
			}
			else if(m_swipe.swipeDirecttion == SWIPE_UP)
			{
			    newScreenIdx = m_screens[m_app_ctx.current_screen].up;
			}
			else if(m_swipe.swipeDirecttion == SWIPE_DOWN)
			{
			    newScreenIdx = m_screens[m_app_ctx.current_screen].down;
			}
		}
		if(newScreenIdx != m_app_ctx.current_screen)
		{
            WidgetRemove(m_screens[m_app_ctx.current_screen].widget);
            WidgetAdd(WIDGET_ROOT, m_screens[newScreenIdx].widget);
            WidgetPaint(WIDGET_ROOT);
            m_app_ctx.current_screen = newScreenIdx;

		}
	}
	m_swipe.swipeDirecttion = SWIPE_NONE;
}




//*****************************************************************************
//
// The interrupt handler for the for Systick interrupt.
// m_global_counter_sec updated 5 times per second
//
//*****************************************************************************
static volatile uint32_t m_global_counter_sec = 0;
void SysTickIntHandler(void)
{
	m_global_counter_sec++;

	//for tests
/*
	if(m_global_counter_sec % 100) //every 20s
	{
		if(esp8266CommandCIPSTATUS())
		{
			updateWifiConnectionStatus(WIFI_CONNECTED);
		}
		else
		{
			updateWifiConnectionStatus(WIFI_NOT_CONNECTED);
		}
	}
	*/
	if(m_global_counter_sec % 100) //every 20s
	{
		if(esp8266CommandCIPSTATUS())
		{
			updateWifiConnectionStatus(WIFI_CONNECTED);
		}
		else
		{
			updateWifiConnectionStatus(WIFI_NOT_CONNECTED);
		}
	}
}



// Main method of the application
int main(void)
{
	static uint32_t touch_screen_pressed_time = 0;

	//FPU
	FPUEnable();
	FPULazyStackingEnable();
	// Setup the system clock to run at 80 Mhz from PLL with crystal reference
	SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

	//Reads from non-volatile memory
	readConfigAndSetAppContext();

	//Debug Console
	debugConsoleInit();

	//Display driver
	ILI9320Init();

	//Read Configuration
	configInit();

	//Wifi client init
	wifiInit(m_app_ctx.eeprom_params.wifi_config[m_app_ctx.flash_params.currentWifiConfig].ap_ssid,
			 m_app_ctx.eeprom_params.wifi_config[m_app_ctx.flash_params.currentWifiConfig].ap_wpa2_pass);

	//UI
    GrContextInit(&m_drawing_context, &g_ILI9320);
    uiInit(&m_drawing_context);
    uiFrameDraw(&m_drawing_context, "Meteo Ubiad Stacja");
    m_app_ctx.current_screen = SCREEN_WIFI_SETTINGS;
    WidgetAdd(WIDGET_ROOT, m_screens[m_app_ctx.current_screen].widget);
    WidgetPaint(WIDGET_ROOT);

	//touchScreenControler
	touchScreenInit();

    // Set the touch screen event handler.
	touchScreenSetTouchCallback(touchScreenCallback);

	//do touch screen calibration if needed
	setTouchScreenCalibration();

	//Enable all interrupts
	ENABLE_ALL_INTERRUPTS();

	// Enable the SysTick and its Interrupt.
	SysTickPeriodSet(SysCtlClockGet()); //1 [s];
	SysTickIntEnable();
	SysTickEnable();

	while (1) {

        handleMovement();
        // Process any messages in the widget message queue.
        WidgetMessageQueueProcess();
#if 0
		if(!ADS7843getIntPinState()) //if touch panel is being touched
		{
			ADS7843read();
			TouchPoint a;
			a = ADS7843getTouchedPoint();
			//MAIN_DEBUG("RESULTS: x=%d, y=%d\n\r", a.x, a.y);
			GrContextForegroundSet(&m_drawing_context, ClrRed);
			GrCircleFill(&m_drawing_context, a.x, a.y, 3);
		}

#endif

		if(!ADS7843getIntPinState()) // if touch panel is being touched)
		{
			if(touch_screen_pressed_time == 0) // first press
			{
				touch_screen_pressed_time = m_global_counter_sec;
			}
			if((m_global_counter_sec - touch_screen_pressed_time) > 50) // if > ~10s do scrreen calibration
			{
				DISABLE_ALL_INTERRUPTS();
				uiClearBackground(&m_drawing_context);
				delay_ms(1500);
				configEepromSetModified(&(m_app_ctx.eeprom_params));
				setTouchScreenCalibration();
				delay_ms(1000);
				SysCtlReset(); // reboot
			}
		}
		else
		{
			touch_screen_pressed_time = 0;
		}
		debugCommandReceived();
	}

}
