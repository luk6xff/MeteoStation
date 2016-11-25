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
	CONN_STATE_NOT_CONECTED,
	CONN_STATE_NOT_CONNECTED_TO_ESP8266,
	CONN_STATE_CONNECTED_TO_ESP8266,
	CONN_STATE_NOT_CONNECTED_TO_SENSOR,
	CONN_STATE_CONNECTED_TO_SENSOR,
	CONN_STATE_WAIT_DATA_FROM_ESP8266,
	CONN_STATE_WAIT_DATA_FROM_SENSOR
}ConnectionState;

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
	uint8_t right;
}ScreenContainer;





//*****************************************************************************
//
// Forward reference to various widget structures.
//
//*****************************************************************************
extern tCanvasWidget g_sMainBackground;
extern tCanvasWidget g_sSettingsPanel;
extern tCanvasWidget g_sKeyboardBackground;
extern tPushButtonWidget g_sPushBtn;


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
// Application globals
//
//*****************************************************************************
static tContext g_sContext;
static volatile State g_mainState = STATE_RESET;
static volatile ConnectionState g_connState = CONN_STATE_NOT_CONECTED;
static volatile Screens g_currentScreen = SCREEN_MAIN;

static ScreenContainer g_sScreens[SCREEN_NUM_OF_SCREENS] =
{
    {
        (tWidget *)&g_sMainBackground,
        SCREEN_MAIN, SCREEN_CONN_SETTINGS, SCREEN_MAIN, SCREEN_MAIN
    },
    {
        (tWidget *)&g_sSettingsPanel,
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
static void onCustomEnable(tWidget *psWidget);
static void drawToggle(const tButtonToggle *psButton, bool bOn);
static void onCustomEntry(tWidget *psWidget);
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
	ConfigParameters* cfg = configGetCurrent();
	if(cfg->touchScreenParams.isUpdated == 0)
	{
		performThreePointCalibration(ctx, &coeffs);
		ADS7843setCalibrationCoefficients(&coeffs);
		cfg->touchScreenParams.calibCoeffs = coeffs;
		cfg->touchScreenParams.isUpdated = 0xFF;
		configSave();

		debugConsolePrintf("COEFFSa: a.x=%d, a.y=%d\n\r", coeffs.m_ax, coeffs.m_ay);
		debugConsolePrintf("COEFFSb: b.x=%d, b.y=%d\n\r", coeffs.m_bx, coeffs.m_by);
		debugConsolePrintf("COEFFSd: d.x=%d, d.y=%d\n\r", coeffs.m_dx, coeffs.m_dy);
		ret= true;
	}
	else
	{
		ADS7843setCalibrationCoefficients(&configGetCurrent()->touchScreenParams.calibCoeffs);
	}

	if(!intsOff)
	{
		IntMasterEnable();
	}
	return ret;
}
//*****************************************************************************
//
// Forward reference to the button press handler.
//
//*****************************************************************************
void OnButtonPress(tWidget *psWidget);

//*****************************************************************************
//
// The canvas widget acting as the main widget of the device
//
//*****************************************************************************
/*
Canvas(g_Background, WIDGET_ROOT, 0, &g_PushBtn,
	   &g_ILI9320, 10, 25, 300, (240 - 25 -10),
       CANVAS_STYLE_FILL, ClrAzure, 0, 0, 0, 0, 0, 0);

RectangularButton(g_PushBtn, &g_Background, 0, 0,
		 	 	  &g_ILI9320, 60, 60, 200, 40,
                  (PB_STYLE_OUTLINE | PB_STYLE_TEXT_OPAQUE | PB_STYLE_TEXT |
                  PB_STYLE_FILL | PB_STYLE_PRESSED),
                  ClrDarkBlue, ClrBlue, ClrWhite, ClrWhite,
                  g_psFontCm20b, "Show Welcome", 0, 0, 0, 0, OnButtonPress);

Canvas(g_Hello, &g_PushBtn, 0, 0,
	   &g_ILI9320, 10, 150, 300, 40,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT),
       ClrBlack, 0, ClrWhite, g_psFontCm20, "Hello World!", 0, 0);
*/
// Main page widgets
char g_pcTempHighLow[40]="--/--C";
Canvas(g_sTempHighLow, &g_sMainBackground, 0, 0,
       &g_ILI9320, 120, 195, 70, 30,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_LEFT |
       CANVAS_STYLE_TEXT_TOP | CANVAS_STYLE_TEXT_OPAQUE, BG_COLOR_MAIN,
       ClrWhite, ClrWhite, g_psFontCmss20, g_pcTempHighLow, 0, 0);

char g_pcTemp[40]="--C";
Canvas(g_sTemp, &g_sMainBackground, &g_sTempHighLow, 0,
       &g_ILI9320, 20, 175, 100, 50,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_RIGHT |
       CANVAS_STYLE_TEXT_TOP | CANVAS_STYLE_TEXT_OPAQUE, BG_COLOR_MAIN,
       ClrWhite, ClrWhite, g_psFontCmss48, g_pcTemp, 0, 0);

char g_pcHumidity[40]="Cisnienie: ----hPa";
Canvas(g_sHumidity, &g_sMainBackground, &g_sTemp, 0,
       &g_ILI9320, 20, 140, 160, 25,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_LEFT |
       CANVAS_STYLE_TEXT_OPAQUE, BG_COLOR_MAIN, ClrWhite, ClrWhite,
       g_psFontCmss20, g_pcHumidity, 0, 0);

char g_pcStatus[40];
Canvas(g_sStatus, &g_sMainBackground, &g_sHumidity, 0,
       &g_ILI9320, 20, 110, 160, 25,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_LEFT |
       CANVAS_STYLE_TEXT_OPAQUE, BG_COLOR_MAIN, ClrWhite, ClrWhite,
       g_psFontCmss20, g_pcStatus, 0, 0);

char g_pcCity[40];
Canvas(g_sCityName, &g_sMainBackground, &g_sStatus, 0,
       &g_ILI9320, 20, 40, 240, 25,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_LEFT |
       CANVAS_STYLE_TEXT_OPAQUE, BG_COLOR_MAIN, ClrWhite, ClrWhite,
       g_psFontCmss20, g_pcCity, 0, 0);

Canvas(g_sMainBackground, WIDGET_ROOT, 0, &g_sCityName,
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
// // The canvas widget acting as Settings panel.
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
RectangularButton(g_sCustomEnable, &g_sSettingsPanel, 0, 0,
       &g_ILI9320, 14, 32, 40, 24,
       0, ClrLightGrey, ClrLightGrey, ClrLightGrey,
       ClrBlack, 0, 0, 0, 0, 0 ,0 , onCustomEnable);

//
// The text entry button for the custom city.
//
RectangularButton(g_sCustomCity, &g_sSettingsPanel, &g_sCustomEnable, 0,
       &g_ILI9320, 118, 30, 190, 28,
       PB_STYLE_FILL | PB_STYLE_TEXT | PB_STYLE_RELEASE_NOTIFY, ClrLightGrey,
       ClrLightGrey, ClrWhite, ClrGray, g_psFontCmss14,
       0, 0, 0, 0 ,0 , onCustomEntry);

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
                g_i32StringWidth = GrStringWidthGet(&g_sContext, g_pcKeyStr,
                                                    40);
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
                WidgetRemove(g_sScreens[g_currentScreen].widget);

                //
                // Switch back to the previous screen and add its widget back.
                //
                g_currentScreen = SCREEN_CONN_SETTINGS;
                WidgetAdd(WIDGET_ROOT, g_sScreens[g_currentScreen].widget);


                //
                // If returning to the main screen then re-draw the frame to
                // indicate the main screen.
                //
                if(g_currentScreen  == SCREEN_MAIN)
                {
                    WidgetPaint(g_sScreens[g_currentScreen].widget);
                }
                else
                {
                    //
                    // Returning to the settings screen.
                    //
                    //FrameDraw(&g_g_sContext, "Settings");
                    WidgetPaint(g_sScreens[g_currentScreen].widget);
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
                g_i32StringWidth = GrStringWidthGet(&g_sContext, g_pcKeyStr,
                                                    40);
            }
            break;
        }
    }
}



//*****************************************************************************
//
// Handles when the custom text area is pressed.
//
//*****************************************************************************
static void onCustomEntry(tWidget *psWidget)

{
    //
    // Only respond if the custom has been enabled.
    //
    //if(g_sConfig.bCustomEnabled)
    //{
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
        WidgetRemove(g_sScreens[g_currentScreen].widget);

        //
        // Activate the keyboard.
        //
        g_currentScreen = SCREEN_KEYBOARD;
        WidgetAdd(WIDGET_ROOT, g_sScreens[g_currentScreen].widget);

        //
        // Clear the main screen area with the settings background color.
        //
        GrContextForegroundSet(&g_sContext, BG_COLOR_SETTINGS);
        clearBackground(&g_sContext);

        GrContextFontSet(&g_sContext, g_psFontCmss24);
        WidgetPaint((tWidget *)&g_sKeyboardBackground);
    //}
}
/*
bool g_bHelloVisible = false;

void OnButtonPress(tWidget *psWidget)
{
    g_bHelloVisible = !g_bHelloVisible;

    if(g_bHelloVisible)
    {
        //
        // Add the Hello widget to the tree as a child of the push
        // button.  We could add it elsewhere but this seems as good a
        // place as any.  It also means we can repaint from g_PushBtn and
        // this will paint both the button and the welcome message.
        //
        WidgetAdd((tWidget *)&g_sPushBtn, (tWidget *)&g_Hello);

        //
        // Change the button text to indicate the new function.
        //
        PushButtonTextSet(&g_PushBtn, "Hide Welcome");

        //
        // Repaint the pushbutton and all widgets beneath it (in this case,
        // the welcome message).
        //
        WidgetPaint((tWidget *)&g_PushBtn);
    }
    else
    {
        //
        // Remove the Hello widget from the tree.
        //
        WidgetRemove((tWidget *)&g_Hello);

        //
        // Change the button text to indicate the new function.
        //
        PushButtonTextSet(&g_PushBtn, "Show Welcome");

        //
        // Repaint the widget tree to remove the Hello widget from the
        // display.  This is rather inefficient but saves having to use
        // additional widgets to overpaint the area of the Hello text (since
        // disabling a widget does not automatically erase whatever it
        // previously displayed on the screen).
        //
        WidgetPaint(W

*/
//*****************************************************************************
//
// The interrupt handler for the for Systick interrupt.
//
//*****************************************************************************
static int uartCounter = 0;
void SysTickIntHandler(void) {
	uartCounter++;

}


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



    tRectangle sRect;
    GrContextInit(&g_sContext, &g_ILI9320);
    //FrameDraw(&g_sContext, "hello-widget");
#if 1
    //
    // Fill the top 24 rows of the screen with blue to create the banner.
    //
    sRect.i16XMin = 0;
    sRect.i16YMin = 0;
    sRect.i16XMax = GrContextDpyWidthGet(&g_sContext) - 1;
    sRect.i16YMax = 23;
    GrContextForegroundSet(&g_sContext, ClrDarkBlue);
    GrRectFill(&g_sContext, &sRect);

    //
    // Put a Red box around the banner.
    //
    GrContextForegroundSet(&g_sContext, ClrRed);
    GrRectDraw(&g_sContext, &sRect);

    // Put the application name in the middle of the banner.
    GrContextForegroundSet(&g_sContext, ClrYellowGreen);
    GrContextFontSet(&g_sContext, &g_sFontCm20);
    GrStringDrawCentered(&g_sContext, "Meteo Stacja", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2, 8, 0);
#endif
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sMainBackground);
    WidgetPaint(WIDGET_ROOT);

	//touchScreenControler
	touchScreenInit();

	//wifi module
    //esp8266Init();

    // Set the touch screen event handler.
	touchScreenSetTouchCallback(WidgetPointerMessage);

	//Enable all interrupts
	IntMasterEnable();

	// Enable the SysTick and its Interrupt.
	SysTickPeriodSet(SysCtlClockGet());
	SysTickIntEnable();
	SysTickEnable();

	//do touch screen calibration if needed
	performTouchScreenCalibration(&g_sContext);

	while (1) {

        // Process any messages in the widget message queue.
        WidgetMessageQueueProcess();
#if 0
		if(!ADS7843getIntPinState()) //if touch panel is being touched
		{
			ADS7843read();
			TouchPoint a;
			a = ADS7843getTouchedPoint();
			//debugConsolePrintf("RESULTS: x=%d, y=%d\n\r", a.x, a.y);
			GrContextForegroundSet(&g_sContext, ClrRed);
			GrCircleFill(&g_sContext, a.x, a.y, 3);
		}

#endif
	}

}
