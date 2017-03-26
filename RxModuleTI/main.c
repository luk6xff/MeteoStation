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
#include "utils/sine.h"


#include "ILI9320_driver.h"
#include "3PointCalibration.h"
#include "config.h"
#include "esp8266.h"
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
	WIFI_NOT_CONNECTED,
	WIFI_CONNECTED,
	WIFI_WAIT_FOR_DATA,
}WifiConnectionState;

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
	//
	//
	//
	#define  MIN_SWIPE_DIFFERENCE 60
	#define  LAST_VAL_BUF_SIZE 10
	int32_t initX;
	int32_t initY;
	int32_t bufX[LAST_VAL_BUF_SIZE];
	int32_t bufY[LAST_VAL_BUF_SIZE];
	uint8_t sampleNum;
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
	bool wifiEnabled;
	bool sensorEnabled;
	bool powerSaveModeEnabled;
	bool swipeEnabled;
	WifiConnectionState wifiState;
	SensorConnectionState sensorState;
	Screens currentScreen;
	char currentCity[50];
	char apSsid[50];
	char apPass[50];

}AppContext;

//*****************************************************************************
//
// Application globals
//
//*****************************************************************************
static tContext g_drawingContext;
static volatile State g_mainState = STATE_RESET;
static volatile Swipe g_swipe;
static volatile AppContext g_appCtx = {false, false, false, true, WIFI_NOT_CONNECTED, SENSOR_NOT_CONNECTED, SCREEN_MAIN, {"NowySacz"}, {"INTEHNET"}, {"Faza939290"}/*(void*)0, (void*)0, (void*)0*/};

static ScreenContainer g_screens[SCREEN_NUM_OF_SCREENS] =
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
static void onKeyEvent(tWidget *psWidget, uint32_t ui32Key, uint32_t ui32Event);
static void onWifiEnable(tWidget *psWidget);
static void onCityEntry(tWidget *psWidget);
static void onSsidEntry(tWidget *psWidget);
static void onPassEntry(tWidget *psWidget);
static void onUpdateTimeEntry(tWidget *psWidget);
static void onParameterEdited(const Screens prevWidget);
static int32_t touchScreenCallback(uint32_t msg, int32_t x, int32_t y);

//*****************************************************************************
//
// Setters for App Context params
//
//*****************************************************************************
static void updateWifiConnectionStatus(WifiConnectionState state);
static void updateSensorConnectionStatus(SensorConnectionState state);

// Clears the main screens background.
static void clearBackground(tContext *psContext)
{
    static const tRectangle sRect =
    {
        BG_MIN_X,
        BG_MIN_Y,
        BG_MAX_X,
        BG_MAX_Y,
    };
    GrRectFill(psContext, &sRect);
}

static void handleMovement(void);

//*****************************************************************************
//
// Touch screen calibration method.
//
//*****************************************************************************
static bool performTouchScreenCalibration(tContext* ctx)
{
	CalibCoefficients coeffs;
	bool ret = false;
	tBoolean intsOff;
	//disable all interrupts
	intsOff = IntMasterDisable();
	//ConfigParameters* cfg = configGetCurrent(); //FLASH
	ConfigEepromParameters* cfg = configEepromGetCurrent();
	if(cfg->touchScreenParams.isModified == 0xFF || cfg->touchScreenParams.isModified == 0x00)
	{
		performThreePointCalibration(ctx, &coeffs);
		ADS7843setCalibrationCoefficients(&coeffs);
		if(confirmThreePointCalibration(ctx))
		{
			cfg->touchScreenParams.calibCoeffs = coeffs;
			cfg->touchScreenParams.isModified = 0x1;
			//configSave(); //FLASH
			configEepromSave();

			MAIN_DEBUG("COEFFSa: a.x=%d, a.y=%d\n\r", coeffs.m_ax, coeffs.m_ay);
			MAIN_DEBUG("COEFFSb: b.x=%d, b.y=%d\n\r", coeffs.m_bx, coeffs.m_by);
			MAIN_DEBUG("COEFFSd: d.x=%d, d.y=%d\n\r", coeffs.m_dx, coeffs.m_dy);
		}
		else
		{
			MAIN_DEBUG("Touch Screen Calibration failed\n\r");
		}

		ret= true;
	}
	else
	{
		ADS7843setCalibrationCoefficients(&cfg->touchScreenParams.calibCoeffs);
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
// // The canvas widget acting as WIFI connection settings panel.
//
//*****************************************************************************

//
// The text entry button for the custom city.
//

RectangularButton(g_wifiApSsid, &g_screenWifiSetupBackground, 0, 0,
       &g_ILI9320, 118, 30, 190, 28,
       PB_STYLE_FILL | PB_STYLE_TEXT, ClrLightGrey,
       ClrLightGrey, ClrWhite, ClrGray, g_psFontCmss14,
	   g_appCtx.apSsid, 0, 0, 0 ,0 , onSsidEntry);

RectangularButton(g_wifiApPass, &g_screenWifiSetupBackground, &g_wifiApSsid, 0,
       &g_ILI9320, 118, 70, 190, 28,
       PB_STYLE_FILL | PB_STYLE_TEXT, ClrLightGrey,
       ClrLightGrey, ClrWhite, ClrGray, g_psFontCmss14,
	   g_appCtx.apPass, 0, 0, 0 ,0 , onPassEntry);

RectangularButton(g_wifiCustomCity, &g_screenWifiSetupBackground, &g_wifiApPass, 0,
       &g_ILI9320, 118, 110, 190, 28,
       PB_STYLE_FILL | PB_STYLE_TEXT, ClrLightGrey,
       ClrLightGrey, ClrWhite, ClrGray, g_psFontCmss14,
	   g_appCtx.currentCity, 0, 0, 0 ,0 , onCityEntry);

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
    		g_appCtx.wifiEnabled = enabled;
    }
    else if(strcmp(g_connCheckBoxes[idx].pcText, "Sensors") == 0)
    {
    		g_appCtx.sensorEnabled = enabled;
    }
    else if(strcmp(g_connCheckBoxes[idx].pcText, "PowerSaving") == 0)
    {
    		g_appCtx.powerSaveModeEnabled = enabled;
    }
}

void onConnToAP(tWidget *psWidget)
{
	if(!g_appCtx.wifiEnabled)
		return;
	WifiConnectionState state = g_appCtx.wifiState;
	if(state == WIFI_NOT_CONNECTED)
	{
		if(!esp8266CommandAT())
		{
			MAIN_DEBUG("ESP8266 not responding, check hardware!");
		}
		else
		{
			esp8266CommandCWMODE(ESP8266_MODE_CLIENT);
			if(esp8266CommandCWJAP(g_appCtx.apSsid, g_appCtx.apPass))
			{
				MAIN_DEBUG("Connected to AP: %s", g_appCtx.apSsid);
				state = WIFI_CONNECTED;
			}
			else
			{
				MAIN_DEBUG("Cannot connect to AP: %s", g_appCtx.apSsid);
			}
		}
	}
	else if(state == WIFI_CONNECTED)
	{
		if(esp8266CommandCWQAP())
		{
			MAIN_DEBUG("Disconnected from AP: %s", g_appCtx.apSsid);
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
	if(g_appCtx.wifiEnabled)
	{
        WidgetRemove(g_screens[g_appCtx.currentScreen].widget);
        g_appCtx.currentScreen = SCREEN_WIFI_SETTINGS;
        WidgetAdd(WIDGET_ROOT, g_screens[g_appCtx.currentScreen].widget);
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
	//if(state == g_appCtx.wifiState)
	//{
	//	return;
	//}
	switch (state) {
		case WIFI_NOT_CONNECTED:
			if(g_appCtx.currentScreen == SCREEN_CONN_SETTINGS)
			{
			    GrContextFontSet(&g_drawingContext, &g_sFontCm12);
			    GrContextForegroundSet(&g_drawingContext, ClrWhite);
			    GrStringDrawCentered(&g_drawingContext, connStateDesc[WIFI_NOT_CONNECTED], -1, 250, 130, true);
			    g_connToApButton.pcText = connStateDesc[WIFI_WAIT_FOR_DATA+1];
			}
			break;
		case WIFI_CONNECTED:
			if(g_appCtx.currentScreen == SCREEN_CONN_SETTINGS)
			{
			    GrContextFontSet(&g_drawingContext, &g_sFontCm16);
			    GrContextForegroundSet(&g_drawingContext, ClrWhite);
			    GrStringDrawCentered(&g_drawingContext, connStateDesc[WIFI_CONNECTED], -1, 250, 130, true);
			    g_connToApButton.pcText = connStateDesc[WIFI_WAIT_FOR_DATA+2];
			}
			break;
		case WIFI_WAIT_FOR_DATA:
			if(g_appCtx.currentScreen == SCREEN_CONN_SETTINGS)
			{
			    GrContextFontSet(&g_drawingContext, &g_sFontCm16);
			    GrContextForegroundSet(&g_drawingContext, ClrWhite);
			    GrStringDrawCentered(&g_drawingContext, connStateDesc[WIFI_WAIT_FOR_DATA], -1, 250, 130, true);
			}
			break;
		default:
			break;
	}
	g_appCtx.wifiState = state;
}

static void updateSensorConnectionStatus(SensorConnectionState state)
{

}




static void onWifiEnable(tWidget *psWidget)
{
	if(g_appCtx.wifiEnabled)
	{
		g_appCtx.wifiEnabled = false;
		PushButtonTextColorSet(&g_wifiCustomCity, ClrGray);
	}
	else
	{
		g_appCtx.wifiEnabled = true;
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
        g_appCtx.swipeEnabled = false;
        WidgetRemove(g_screens[g_appCtx.currentScreen].widget);
        uiKeyboardCreate(g_appCtx.currentCity, g_appCtx.currentScreen,
        				"Save the city", "Wanna save the city?",
						onParameterEdited);
        // Activate the keyboard.
        g_appCtx.currentScreen = SCREEN_KEYBOARD;
}

static void onSsidEntry(tWidget *psWidget)
{
}

static void onPassEntry(tWidget *psWidget)
{
}

static void onUpdateTimeEntry(tWidget *psWidget)
{
}

//*****************************************************************************
// @brief Function which is passed to given to react on a parameter change (keyboard widget exit)
//*****************************************************************************
static void onParameterEdited(const Screens prevWidget)
{
	g_appCtx.currentScreen = prevWidget;
    WidgetAdd(WIDGET_ROOT, g_screens[g_appCtx.currentScreen].widget);
	WidgetPaint(WIDGET_ROOT);
    //Enable swiping after removing keyboard from the root
    g_appCtx.swipeEnabled = true;
}


//*****************************************************************************
//
// The interrupt handler for the for Systick interrupt.
//
//*****************************************************************************
static int g_uartCounter = 0;
void SysTickIntHandler(void)
{
	g_uartCounter++;
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
    if(g_appCtx.swipeEnabled)
    {
        switch(msg)
        {
            // The user has just touched the screen.
            case WIDGET_MSG_PTR_DOWN:
            {
                //
                // Save this press location.
                //
            	if(!g_swipe.swipeOnGoing)
            	{
					g_swipe.initX = x;
					g_swipe.initY = y;
					g_swipe.swipeOnGoing = true;
            	}
            	else
            	{	++g_swipe.sampleNum;
            		g_swipe.bufX[g_swipe.sampleNum % LAST_VAL_BUF_SIZE] = x;
            		g_swipe.bufY[g_swipe.sampleNum % LAST_VAL_BUF_SIZE] = y;
            	}

                break;
            }

            // The user has moved the touch location on the screen.
            case WIDGET_MSG_PTR_MOVE:
            {
            	if(g_swipe.swipeOnGoing)
            	{
            		++g_swipe.sampleNum;
            		g_swipe.bufX[g_swipe.sampleNum % LAST_VAL_BUF_SIZE] = x;
            		g_swipe.bufY[g_swipe.sampleNum % LAST_VAL_BUF_SIZE] = y;

            	}
                break;
            }

            // The user just stopped touching the screen.
            case WIDGET_MSG_PTR_UP:
            {
            	if(g_swipe.swipeOnGoing)
            	{
            		//checks on last gathered data for now
            		int32_t xLastVal = g_swipe.bufX[g_swipe.sampleNum % LAST_VAL_BUF_SIZE];
            		int32_t yLastVal = g_swipe.bufY[g_swipe.sampleNum % LAST_VAL_BUF_SIZE];
            		bool xLessThanInit = xLastVal < g_swipe.initX;
            		bool yLessThanInit = yLastVal < g_swipe.initY;
            		swipeDiffX = ((xLastVal - g_swipe.initX)>0) ? (xLastVal - g_swipe.initX) : (g_swipe.initX - xLastVal);
            		swipeDiffY = ((yLastVal - g_swipe.initY)>0) ? (yLastVal - g_swipe.initY) : (g_swipe.initY - yLastVal);
            		// checks which difference is bigger
            		if(swipeDiffX > swipeDiffY )
            		{

            			if(!xLessThanInit && (swipeDiffX > MIN_SWIPE_DIFFERENCE))
						{
							g_swipe.swipeDirecttion = SWIPE_RIGHT;
						}
						else if(xLessThanInit && (swipeDiffX > MIN_SWIPE_DIFFERENCE))
						{
							g_swipe.swipeDirecttion = SWIPE_LEFT;
						}
            		}
            		else
            		{
						if(!yLessThanInit && (swipeDiffY > MIN_SWIPE_DIFFERENCE))
						{
							g_swipe.swipeDirecttion = SWIPE_DOWN;
						}
						else if(yLessThanInit && (swipeDiffY > MIN_SWIPE_DIFFERENCE))
						{
							g_swipe.swipeDirecttion = SWIPE_UP;
						}
            		}
            	}
        		g_swipe.swipeOnGoing = false;
        		g_swipe.sampleNum = 0;
        		break;
            }
        }
    }
    WidgetPointerMessage(msg, x, y);

    return(0);
}


static void handleMovement(void)
{
	uint16_t newScreenIdx = g_appCtx.currentScreen;
	if(g_appCtx.swipeEnabled)
	{
		if(g_swipe.swipeDirecttion != SWIPE_NONE )
		{

			if(g_swipe.swipeDirecttion == SWIPE_RIGHT)
			{
			    newScreenIdx = g_screens[g_appCtx.currentScreen].right;
			}
			else if(g_swipe.swipeDirecttion == SWIPE_LEFT)
			{
			    newScreenIdx = g_screens[g_appCtx.currentScreen].left;
			}
			else if(g_swipe.swipeDirecttion == SWIPE_UP)
			{
			    newScreenIdx = g_screens[g_appCtx.currentScreen].up;
			}
			else if(g_swipe.swipeDirecttion == SWIPE_DOWN)
			{
			    newScreenIdx = g_screens[g_appCtx.currentScreen].down;
			}
		}
		if(newScreenIdx != g_appCtx.currentScreen)
		{
            WidgetRemove(g_screens[g_appCtx.currentScreen].widget);
            WidgetAdd(WIDGET_ROOT, g_screens[newScreenIdx].widget);
            WidgetPaint(WIDGET_ROOT);
            g_appCtx.currentScreen = newScreenIdx;

		}
	}
	g_swipe.swipeDirecttion = SWIPE_NONE;

}



// Main method of the application

int main(void)
{
	FPUEnable();
	FPULazyStackingEnable();
	// Setup the system clock to run at 80 Mhz from PLL with crystal reference
	SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, RED_LED | BLUE_LED | GREEN_LED);
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GREEN_LED);
	GPIOPinWrite(GPIO_PORTF_BASE, RED_LED, 1 & 0xFF ? RED_LED : 0);
	GPIOPinWrite(GPIO_PORTF_BASE, BLUE_LED, 1 & 0xFF ? BLUE_LED : 0);
	GPIOPinWrite(GPIO_PORTF_BASE, GREEN_LED, 1 & 0xFF ? GREEN_LED : 0);
	GPIOPinWrite(GPIO_PORTF_BASE, BLUE_LED, 0 & 0xFF ? BLUE_LED : 0);

	//debug Console
	debugConsoleInit();

	//Display driver
	ILI9320Init();

	//Configuration
	configInit();

	//ESP8266
	esp8266Init();


    tRectangle sRect;
    GrContextInit(&g_drawingContext, &g_ILI9320);
    uiInit(&g_drawingContext);
    //FrameDraw(&g_drawingContext, "hello-widget");
#if 1
    //
    // Fill the top 24 rows of the screen with blue to create the banner.
    //
    sRect.i16XMin = 0;
    sRect.i16YMin = 0;
    sRect.i16XMax = GrContextDpyWidthGet(&g_drawingContext) - 1;
    sRect.i16YMax = 23;
    GrContextForegroundSet(&g_drawingContext, ClrDarkBlue);
    GrRectFill(&g_drawingContext, &sRect);

    //
    // Put a Red box around the banner.
    //
    GrContextForegroundSet(&g_drawingContext, ClrRed);
    GrRectDraw(&g_drawingContext, &sRect);

    // Put the application name in the middle of the banner.
    GrContextForegroundSet(&g_drawingContext, ClrYellowGreen);
    GrContextFontSet(&g_drawingContext, &g_sFontCm20);
    GrStringDrawCentered(&g_drawingContext, "Meteo Ubiad Stacja", -1,
                         GrContextDpyWidthGet(&g_drawingContext) / 2, 8, 0);
#endif


    g_appCtx.currentScreen = SCREEN_WIFI_SETTINGS;
    WidgetAdd(WIDGET_ROOT, g_screens[g_appCtx.currentScreen].widget);
    WidgetPaint(WIDGET_ROOT);

	//touchScreenControler
	touchScreenInit();

    // Set the touch screen event handler.
	touchScreenSetTouchCallback(touchScreenCallback);

	//Enable all interrupts
	ENABLE_ALL_INTERRUPTS();

	// Enable the SysTick and its Interrupt.
	SysTickPeriodSet(SysCtlClockGet());
	SysTickIntEnable();
	SysTickEnable();

	//do touch screen calibration if needed
	performTouchScreenCalibration(&g_drawingContext);


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
			GrContextForegroundSet(&g_drawingContext, ClrRed);
			GrCircleFill(&g_drawingContext, a.x, a.y, 3);
		}

#endif
		debugCommandReceived();
	}

}
