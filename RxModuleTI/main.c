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

#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "grlib/pushbutton.h"
#include "grlib/checkbox.h"
#include "grlib/container.h"
#include "grlib/radiobutton.h"
#include "grlib/slider.h"
#include "grlib/keyboard.h"

#include "ILI9320_driver.h"
#include "3PointCalibration.h"
#include "config.h"
#include "debugConsole.h"
#include "esp8266.h"
#include "touch.h"
#include "images.h"

#define RED_LED   GPIO_PIN_1
#define BLUE_LED  GPIO_PIN_2
#define GREEN_LED GPIO_PIN_3



//*****************************************************************************
//
// Defines for the main screen area used by the application.
//
//*****************************************************************************
#define BG_MIN_X                4
#define BG_MAX_X                (320 - 4)
#define BG_MIN_Y                24
#define BG_MAX_Y                (240 - 8)
#define BG_COLOR_SETTINGS       ClrGray
#define BG_COLOR_MAIN           ClrBlack


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

//*****************************************************************************
//
// Screens
//
//*****************************************************************************
typedef enum
{
	SCREEN_MAIN,
	SCREEN_CONN_SETTINGS,
	SCREEN_KEYBOARD,
	SCREEN_NUM_OF_SCREENS
}Screens;

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
extern tCanvasWidget g_sKeyboardBackground;
extern tCanvasWidget g_settingsPanel;
extern tPushButtonWidget g_sPushBtn;

extern tCanvasWidget g_panels[];


//*****************************************************************************
//
// Typedefs.
//
//*****************************************************************************
//*****************************************************************************
//
// State information for the toggle buttons used in the settings panel.
//
//*****************************************************************************
typedef struct
{
    //
    // The outside area of the button.
    //
    tRectangle sRectContainer;

    //
    // The actual button area.
    //
    tRectangle sRectButton;

    //
    // The text for the on position.
    //
    const char *pcOn;

    //
    // The text for the off position.
    //
    const char *pcOff;

    //
    // The label for the button.
    //
    const char *pcLabel;
}
tButtonToggle;


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
	char* currentCity;

}AppContext;


//*****************************************************************************
//
// Application globals
//
//*****************************************************************************
static tContext g_context;
static volatile State g_mainState = STATE_RESET;
static volatile Swipe g_swipe;
static volatile AppContext g_appCtx = {false, false, false, true, WIFI_NOT_CONNECTED, SENSOR_NOT_CONNECTED, SCREEN_MAIN, (void*)0};

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
        (tWidget *)&g_sKeyboardBackground,
        SCREEN_KEYBOARD, SCREEN_KEYBOARD, SCREEN_KEYBOARD, SCREEN_KEYBOARD
    }
};



//*****************************************************************************
//
// Methods forward declarations.
//
//*****************************************************************************
static void onKeyEvent(tWidget *psWidget, uint32_t ui32Key, uint32_t ui32Event);
static void onWifiEnable(tWidget *psWidget);
static void drawToggleButton(const tButtonToggle *psButton, bool enable);
static void onCityEntry(tWidget *psWidget);
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
	if(cfg->touchScreenParams.isUpdated == 0xFF || cfg->touchScreenParams.isUpdated == 0x00)
	{
		performThreePointCalibration(ctx, &coeffs);
		ADS7843setCalibrationCoefficients(&coeffs);
		if(confirmThreePointCalibration(ctx))
		{
			cfg->touchScreenParams.calibCoeffs = coeffs;
			cfg->touchScreenParams.isUpdated = 0x1;
			//configSave(); //FLASH
			configEepromSave();

			debugConsolePrintf("COEFFSa: a.x=%d, a.y=%d\n\r", coeffs.m_ax, coeffs.m_ay);
			debugConsolePrintf("COEFFSb: b.x=%d, b.y=%d\n\r", coeffs.m_bx, coeffs.m_by);
			debugConsolePrintf("COEFFSd: d.x=%d, d.y=%d\n\r", coeffs.m_dx, coeffs.m_dy);
		}
		else
		{
			debugConsolePrintf("Touch Screen Calibration failed\n\r");
		}

		ret= true;
	}
	else
	{
		ADS7843setCalibrationCoefficients(&cfg->touchScreenParams.calibCoeffs);
	}

	if(!intsOff)
	{
		IntMasterEnable();
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
// The canvas widget acting as Keyboard
//
//*****************************************************************************

//
// Keyboard cursor blink rate.
//
#define KEYBOARD_BLINK_RATE     100

//
// The current string pointer for the keyboard.
//
static char *g_pcKeyStr;

//
// The current string index for the keyboard.
//
static uint32_t g_ui32StringIdx;

//
// A place holder string used when nothing is being displayed on the keyboard.
//
static const char g_cTempStr = 0;

//
// The current string width for the keyboard in pixels.
//
static int32_t g_i32StringWidth;

//
// The cursor blink counter.
//
static volatile uint32_t g_ui32CursorDelay;

//
// The keyboard widget used by the application.
//
Keyboard(g_sKeyboard, &g_sKeyboardBackground, 0, 0,
		 &g_ILI9320, 8, 90, 300, 140,
         KEYBOARD_STYLE_FILL | KEYBOARD_STYLE_AUTO_REPEAT |
         KEYBOARD_STYLE_PRESS_NOTIFY | KEYBOARD_STYLE_RELEASE_NOTIFY |
         KEYBOARD_STYLE_BG,
         ClrBlack, ClrGray, ClrDarkGray, ClrGray, ClrBlack, g_psFontCmss14,
         100, 100, NUM_KEYBOARD_US_ENGLISH, g_psKeyboardUSEnglish, onKeyEvent);

//
// The keyboard text entry area.
//
Canvas(g_sKeyboardText, &g_sKeyboardBackground, &g_sKeyboard, 0,
	   &g_ILI9320, BG_MIN_X, BG_MIN_Y,
       BG_MAX_X - BG_MIN_X, 60,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_LEFT,
       ClrBlack, ClrWhite, ClrWhite, g_psFontCmss24, &g_cTempStr, 0 ,0 );

//
// The full background for the keyboard when it is takes over the screen.
//
Canvas(g_sKeyboardBackground, WIDGET_ROOT, 0, &g_sKeyboardText,
	   &g_ILI9320, BG_MIN_X, BG_MIN_Y,
       BG_MAX_X - BG_MIN_X, BG_MAX_Y - BG_MIN_Y,
       CANVAS_STYLE_FILL, ClrBlack, ClrWhite, ClrWhite, 0, 0, 0 ,0 );


//*****************************************************************************
//
// // The canvas widget acting as WIFI connection settings panel.
//
//*****************************************************************************

//
// The custom city toggle button.
//
const tButtonToggle sCustomToggle =
{
    //
    // Outer border of button.
    //
    {12, 30, 116, 57},

    //
    // Button border.
    //
    {14, 32, 54, 55},

    "On",
    "Off",
    "City"
};

//
// The actual button widget that receives the press events.
//
/*
RectangularButton(g_widgetWifiEnable, &g_sSettingsPanel, 0, 0,
       &g_ILI9320, 14, 32, 40, 24,
       0, ClrLightGrey, ClrLightGrey, ClrLightGrey,
       ClrBlack, 0, 0, 0, 0, 0 ,0 , onWifiEnable);
       */
//static const char* g_wifiEnableText = "Wifi Enable";
/*
char g_wifiEnableText[] = {"Wifi Enable"};
RadioButton(g_widgetWifiEnable, &g_sSettingsPanel, 0, 0,
       &g_ILI9320, 14, 32, 20, 10,
	   RB_STYLE_OUTLINE | RB_STYLE_TEXT | RB_STYLE_SELECTED, 5, ClrBlue, ClrRed, ClrWhite,
	   g_psFontCmss14, g_wifiEnableText, 0, onWifiEnable);

//
// The text entry button for the custom city.
//
RectangularButton(g_sCustomCity, &g_sSettingsPanel, &g_widgetWifiEnable, 0,
       &g_ILI9320, 118, 30, 190, 28,
       PB_STYLE_FILL | PB_STYLE_TEXT | PB_STYLE_RELEASE_NOTIFY, ClrLightGrey,
       ClrLightGrey, ClrWhite, ClrGray, g_psFontCmss14,
       0, 0, 0, 0 ,0 , onCityEntry);

//
// MAC Address display.
//
char g_pcMACAddr[40];
Canvas(g_sMACAddr, &g_sSettingsPanel, &g_sCustomCity, 0,
       &g_ILI9320, 12, 180, 147, 20,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_LEFT,
       ClrGray, ClrDarkGray, ClrBlack, g_psFontCmss14,
       g_pcMACAddr, 0, 0);

//
// IP Address display.
//
char g_pcIPAddr[20];
Canvas(g_sIPAddr, &g_sSettingsPanel, &g_sMACAddr, 0,
       &g_ILI9320, 12, 200, 147, 20,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_LEFT,
       ClrGray, ClrDarkGray, ClrBlack, g_psFontCmss14,
       g_pcIPAddr, 0, 0);

//
// Background of the settings panel.
//
Canvas(g_sSettingsPanel, WIDGET_ROOT, 0, &g_sIPAddr,
       &g_ILI9320, BG_MIN_X, BG_MIN_Y,
       BG_MAX_X - BG_MIN_X, BG_MAX_Y - BG_MIN_Y,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT_RIGHT |
       CANVAS_STYLE_TEXT_TOP, ClrGray, ClrWhite, ClrBlack, 0,
       0, 0, 0);
       */
//TODO
void onConnCheckBoxChange(tWidget *psWidget, uint32_t bSelected);
void onConnToAP(tWidget *psWidget);
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

RectangularButton(g_connToApButton, g_panelConnContainers+1, 0, 0,
				  &g_ILI9320, 200, 52, 100, 28,
				  PB_STYLE_FILL | PB_STYLE_TEXT | PB_STYLE_OUTLINE | PB_STYLE_RELEASE_NOTIFY,
				  ClrGreen, ClrRed, ClrSilver, ClrWhite, g_psFontCmss14,
				  "Connect", 0, 0, 0 ,0 , onConnToAP);

tContainerWidget g_panelConnContainers[] = {
		ContainerStruct(WIDGET_ROOT, g_panelConnContainers + 1, g_connCheckBoxes,
						&g_ILI9320, 8, 24, 180, 148,
						CTR_STYLE_OUTLINE | CTR_STYLE_TEXT, 0, ClrGray, ClrSilver,
						g_psFontCm16, "Connection Setup"),
		ContainerStruct(WIDGET_ROOT, 0, &g_connToApButton,
						&g_ILI9320, 188, 24, 136-8-4, 148,
						CTR_STYLE_OUTLINE | CTR_STYLE_TEXT, 0, ClrGray, ClrSilver,
						g_psFontCm16, "Connect to AP")
};

tCanvasWidget g_settingsPanel =
		CanvasStruct(WIDGET_ROOT, 0, g_panelConnContainers, &g_ILI9320,
		BG_MIN_X, BG_MIN_Y, BG_MAX_X - BG_MIN_X, BG_MAX_Y - BG_MIN_Y,
		CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0);

void onConnCheckBoxChange(tWidget *widget, uint32_t bSelected)
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
                   bSelected ? g_pui8LightOn : g_pui8LightOff);
    WidgetPaint((tWidget *)(g_connCheckBoxIndicators + idx));
}

void onConnToAP(tWidget *psWidget)
{
	WifiConnectionState state = g_appCtx.wifiState;
	//esp8266CommandCIPSTART();
	//updateWifiConnectionStatus(WifiConnectionState state);
	updateWifiConnectionStatus(WIFI_NOT_CONNECTED); //TODO
}

static void updateWifiConnectionStatus(WifiConnectionState state)
{
	if(state == g_appCtx.wifiState)
	{
		return;
	}
	switch (state) {
		case WIFI_NOT_CONNECTED:
			if(g_appCtx.currentScreen == SCREEN_CONN_SETTINGS)
			{
			    GrContextFontSet(&g_context, &g_sFontCm16);
			    GrContextForegroundSet(&g_context, ClrWhite);
			    GrStringDrawCentered(&g_context, "NOT_CONNECTED", -1, 200, 95, true);
			}
			break;
		case WIFI_CONNECTED:
			if(g_appCtx.currentScreen == SCREEN_CONN_SETTINGS)
			{
			    GrContextFontSet(&g_context, &g_sFontCm16);
			    GrContextForegroundSet(&g_context, ClrWhite);
			    GrStringDrawCentered(&g_context, "CONNECTED", -1, 200, 95, true);
			}
			break;
		case WIFI_WAIT_FOR_DATA:
			if(g_appCtx.currentScreen == SCREEN_CONN_SETTINGS)
			{
			    GrContextFontSet(&g_context, &g_sFontCm16);
			    GrContextForegroundSet(&g_context, ClrWhite);
			    GrStringDrawCentered(&g_context, "WAIT_FOR_DATA", -1, 200, 95, true);
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

//*****************************************************************************
//
// Draws a toggle button.
//
//*****************************************************************************
static void drawToggleButton(const tButtonToggle *psButton, bool enable)
{
    tRectangle sRect;
    int16_t i16X, i16Y;

    //
    // Copy the data out of the bounds of the button.
    //
    sRect = psButton->sRectButton;

    GrContextForegroundSet(&g_context, ClrLightGrey);
    GrRectFill(&g_context, &psButton->sRectContainer);

    //
    // Copy the data out of the bounds of the button.
    //
    sRect = psButton->sRectButton;

    GrContextForegroundSet(&g_context, ClrDarkGray);
    GrRectFill(&g_context, &psButton->sRectButton);

    if(enable)
    {
        sRect.i16XMin += 2;
        sRect.i16YMin += 2;
        sRect.i16XMax -= 15;
        sRect.i16YMax -= 2;
    }
    else
    {
        sRect.i16XMin += 15;
        sRect.i16YMin += 2;
        sRect.i16XMax -= 2;
        sRect.i16YMax -= 2;
    }
    GrContextForegroundSet(&g_context, ClrLightGrey);
    GrRectFill(&g_context, &sRect);

    GrContextFontSet(&g_context, &g_sFontCm16);
    GrContextForegroundSet(&g_context, ClrBlack);
    GrContextBackgroundSet(&g_context, ClrLightGrey);

    i16X = sRect.i16XMin + ((sRect.i16XMax - sRect.i16XMin) / 2);
    i16Y = sRect.i16YMin + ((sRect.i16YMax - sRect.i16YMin) / 2);

    if(enable)
    {
        GrStringDrawCentered(&g_context, psButton->pcOn, -1, i16X, i16Y,
                             true);
    }
    else
    {
        GrStringDrawCentered(&g_context, psButton->pcOff, -1, i16X, i16Y,
                             true);
    }

    if(psButton->pcLabel)
    {
        GrStringDraw(&g_context, psButton->pcLabel, -1,
                     psButton->sRectButton.i16XMax + 2,
                     psButton->sRectButton.i16YMin + 6,
                     true);
    }
}




//*****************************************************************************
//
// Handles when a key is pressed on the keyboard.
//
//*****************************************************************************
static void onKeyEvent(tWidget *psWidget, uint32_t ui32Key, uint32_t ui32Event)
{
    switch(ui32Key)
    {
        //
        // Look for a backspace key press.
        //
        case UNICODE_BACKSPACE:
        {
            if(ui32Event == KEYBOARD_EVENT_PRESS)
            {
                if(g_ui32StringIdx != 0)
                {
                    g_ui32StringIdx--;
                    g_pcKeyStr[g_ui32StringIdx] = 0;
                }

                WidgetPaint((tWidget *)&g_sKeyboardText);

                //
                // Save the pixel width of the current string.
                //
                g_i32StringWidth = GrStringWidthGet(&g_context, g_pcKeyStr, 40);
            }
            break;
        }
        //
        // Look for an enter/return key press.  This will exit the keyboard and
        // return to the current active screen.
        //
        case UNICODE_RETURN:
        {
            if(ui32Event == KEYBOARD_EVENT_RELEASE)
            {
                //
                // Get rid of the keyboard widget.
                //
                WidgetRemove(g_screens[g_appCtx.currentScreen].widget);

                //
                // Switch back to the previous screen and add its widget back.
                //
                g_appCtx.currentScreen = SCREEN_CONN_SETTINGS;
                WidgetAdd(WIDGET_ROOT, g_screens[g_appCtx.currentScreen].widget);


                //
                // If returning to the main screen then re-draw the frame to
                // indicate the main screen.
                //
                if(g_appCtx.currentScreen  == SCREEN_MAIN)
                {
                    WidgetPaint(g_screens[g_appCtx.currentScreen].widget);
                }
                else
                {
                    //
                    // Returning to the settings screen.
                    //
                    //FrameDraw(&g_g_context, "Settings");
                    WidgetPaint(g_screens[g_appCtx.currentScreen].widget);
                    //AnimateButtons(true);
                    WidgetMessageQueueProcess();

                }
            }

            break;
        }
        //
        // If the key is not special then update the text string.
        //
        default:
        {
            if(ui32Event == KEYBOARD_EVENT_PRESS)
            {
                // Set the string to the current string to be updated.
                //
                if(g_ui32StringIdx == 0)
                {
                    CanvasTextSet(&g_sKeyboardText, g_pcKeyStr);
                }
                g_pcKeyStr[g_ui32StringIdx] = (char)ui32Key;
                g_ui32StringIdx++;
                g_pcKeyStr[g_ui32StringIdx] = 0;

                WidgetPaint((tWidget *)&g_sKeyboardText);

                //
                // Save the pixel width of the current string.
                //
                g_i32StringWidth = GrStringWidthGet(&g_context, g_pcKeyStr, 40);
            }
            break;
        }
    }
}



static void onWifiEnable(tWidget *psWidget)
{
	if(g_appCtx.wifiEnabled)
	{
		g_appCtx.wifiEnabled = false;
		//PushButtonTextColorSet(&g_sCustomCity, ClrGray);
	}
	else
	{
		g_appCtx.wifiEnabled = true;
		//PushButtonTextColorSet(&g_sCustomCity, ClrBlack);
	}
    drawToggleButton(&sCustomToggle, g_appCtx.wifiEnabled);
   // WidgetPaint((tWidget *)&g_sCustomCity);
}


//*****************************************************************************
//
// Handles when the custom text area is pressed.
//
//*****************************************************************************
static void onCityEntry(tWidget *psWidget)

{
    // if wifi connection is enabled
    if(g_appCtx.wifiEnabled)
    {
        //
        // Disable swiping while the keyboard is active.
        //
        //g_sSwipe.bEnable = false;
        //
        // The keyboard string is now the custom city so set the string,
        // reset the string index and width to 0.
        //
        //g_pcKeyStr = g_sConfig.pcCustomCity;
        g_ui32StringIdx = 0;
        g_i32StringWidth = 0;

        //
        // Set the initial string to a null string so that nothing is shown.
        //
        CanvasTextSet(&g_sKeyboardText, &g_cTempStr);

        //
        // Remove the current widget so that it is not used while keyboard
        // is active.
        //
        WidgetRemove(g_screens[g_appCtx.currentScreen].widget);

        //
        // Activate the keyboard.
        //
        g_appCtx.currentScreen = SCREEN_KEYBOARD;
        WidgetAdd(WIDGET_ROOT, g_screens[g_appCtx.currentScreen].widget);

        //
        // Clear the main screen area with the settings background color.
        //
        GrContextForegroundSet(&g_context, BG_COLOR_SETTINGS);
        clearBackground(&g_context);

        GrContextFontSet(&g_context, g_psFontCmss24);
        WidgetPaint((tWidget *)&g_sKeyboardBackground);
    }
}


//*****************************************************************************
//
// The interrupt handler for the for Systick interrupt.
//
//*****************************************************************************
static int uartCounter = 0;
void SysTickIntHandler(void) {
	uartCounter++;

	// keyboard cursor blinking
    if((g_ui32CursorDelay != 0) &&
       (g_ui32CursorDelay != (KEYBOARD_BLINK_RATE / 2)))
    {
        g_ui32CursorDelay--;
    }

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
            //
            // The user has just touched the screen.
            //
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

            //
            // The user has moved the touch location on the screen.
            //
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



//
// Main method of the application
//

int main(void) {
	//FPUEnable();
	//FPULazyStackingEnable();
	//
	// Setup the system clock to run at 80 Mhz from PLL with crystal reference
	//
	SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
	//SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
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
    GrContextInit(&g_context, &g_ILI9320);
    //FrameDraw(&g_context, "hello-widget");
#if 1
    //
    // Fill the top 24 rows of the screen with blue to create the banner.
    //
    sRect.i16XMin = 0;
    sRect.i16YMin = 0;
    sRect.i16XMax = GrContextDpyWidthGet(&g_context) - 1;
    sRect.i16YMax = 23;
    GrContextForegroundSet(&g_context, ClrDarkBlue);
    GrRectFill(&g_context, &sRect);

    //
    // Put a Red box around the banner.
    //
    GrContextForegroundSet(&g_context, ClrRed);
    GrRectDraw(&g_context, &sRect);

    // Put the application name in the middle of the banner.
    GrContextForegroundSet(&g_context, ClrYellowGreen);
    GrContextFontSet(&g_context, &g_sFontCm20);
    GrStringDrawCentered(&g_context, "Meteo Ubiad Stacja", -1,
                         GrContextDpyWidthGet(&g_context) / 2, 8, 0);
#endif


    g_appCtx.currentScreen = SCREEN_MAIN;
    WidgetAdd(WIDGET_ROOT, g_screens[g_appCtx.currentScreen].widget);
    WidgetPaint(WIDGET_ROOT);

	//touchScreenControler
	touchScreenInit();

    // Set the touch screen event handler.
	touchScreenSetTouchCallback(touchScreenCallback);

	//Enable all interrupts
	IntMasterEnable();

	// Enable the SysTick and its Interrupt.
	SysTickPeriodSet(SysCtlClockGet());
	SysTickIntEnable();
	SysTickEnable();

	//do touch screen calibration if needed
	performTouchScreenCalibration(&g_context);


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
			//debugConsolePrintf("RESULTS: x=%d, y=%d\n\r", a.x, a.y);
			GrContextForegroundSet(&g_context, ClrRed);
			GrCircleFill(&g_context, a.x, a.y, 3);
		}

#endif
		debugCommandReceived();
	}

}
