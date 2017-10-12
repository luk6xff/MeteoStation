#include <string.h>

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"

#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/systick.h"
#include "driverlib/interrupt.h"
#include "driverlib/fpu.h"
#include "driverlib/watchdog.h"


#include "utils/ustdlib.h"

#include "delay.h"
#include "3PointCalibration.h"
#include "config.h"
#include "wifi.h"
#include "ili9320_driver.h"
#include "touch.h"
#include "system.h"
#include "time_lib.h"
#include "wdog.h"
#include "spiCommon.h"
#include "ui/ui_common.h"
#include "RF22.h"

#include "debugConsole.h"
#define MAIN_DEBUG_ENABLE 0
static const char* name = "main";






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
	STATE_GET_WEATHER_FROWEB,
	STATE_GET_WEATHER_FROSENSOR,
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



//*****************************************************************************
//
// Structure that describes swipes data.
//
//*****************************************************************************
typedef struct
{
	#define  SWIPE_MIN_DIFFERENCE 60
	#define  SWIPE_LAST_VAL_BUF_SIZE 10
	int32_t initX;
	int32_t initY;
	int32_t bufX[SWIPE_LAST_VAL_BUF_SIZE];
	int32_t bufY[SWIPE_LAST_VAL_BUF_SIZE];
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
	ConfigEepromParameters eeproparams;
	bool swipe_enabled;
}AppContext;

//*****************************************************************************
//
// Application globals
//
//*****************************************************************************

static volatile State g_mainState = STATE_RESET;
static volatile Swipe swipe;
static AppContext app_ctx;



// period notifiers
// @brief new weather data from wifi
static volatile bool get_new_temp_data = false;
// @brief global counter
static volatile uint32_t global_counter_sec = 0;



//*****************************************************************************
//
// Methods forward declarations.
//
//*****************************************************************************
static int32_t touchScreenCallback(uint32_t msg, int32_t x, int32_t y);

//*****************************************************************************
//
// Private methods
//
//*****************************************************************************

// Reads config from flash and eeprom and fills the current App context
static bool readConfigAndSetAppContext()
{
	// assign defaults
	memcpy(&app_ctx.flash_params, configFlashGetDefaultSettings(), sizeof(ConfigFlashParameters));
	memcpy(&app_ctx.eeproparams, configEepromGetDefaultSettings(), sizeof(ConfigEepromParameters));

	// reads configuration from flash/eeprom
	configInit();

	//configFlashSetInvalid(configFlashGetCurrent());  //new memory layout introduced - comment it out later
	if(configFlashIsInvalid(configFlashGetCurrent()))
	{
		configFlashSaveDefaults();
	}
	else
	{
		// assign defaults settings from memory
		memcpy(&app_ctx.flash_params, configFlashGetCurrent(), sizeof(ConfigFlashParameters));
	}

	//configEepromSetInvalid(configEepromGetCurrent()); //new memory layout introduced - comment it out later
	if(configEepromIsInvalid(configEepromGetCurrent()))
	{
		configEepromSaveDefaults();
	}
	else
	{
		// assign defaults settings from memory
		memcpy(&app_ctx.eeproparams, configEepromGetCurrent(), sizeof(ConfigEepromParameters));
	}
	app_ctx.swipe_enabled = true;
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
	ConfigEepromParameters* cfg = &app_ctx.eeproparams;
	if(configEepromIsInvalid(cfg) || cfg->touch_screen_params.is_valid == 0x00 ||   // first time enabled on the plant
	   configEepromCheckAndCleanModified(cfg))										// if you want manually run the calibration
	{
		performThreePointCalibration(uiGetMainDrawingContext(), &coeffs);
		ADS7843setCalibrationCoefficients(&coeffs);
		if(confirmThreePointCalibration(uiGetMainDrawingContext()))
		{
			cfg->touch_screen_params.calib_coeffs = coeffs;
			cfg->touch_screen_params.is_valid = 0x01;
			cfg->params_version = 0x01;
			configEepromSetModified(cfg);
			configEepromSaveSettingsToMemory(&app_ctx.eeproparams);
			DEBUG(MAIN_DEBUG_ENABLE, name, "COEFFSa: a.x=%d, a.y=%d\n\r", coeffs.a_x, coeffs.a_y);
			DEBUG(MAIN_DEBUG_ENABLE, name, "COEFFSb: b.x=%d, b.y=%d\n\r", coeffs.b_x, coeffs.b_y);
			DEBUG(MAIN_DEBUG_ENABLE, name, "COEFFSd: d.x=%d, d.y=%d\n\r", coeffs.d_x, coeffs.d_y);
		}
		else
		{
			DEBUG(MAIN_DEBUG_ENABLE, name, "Touch Screen Calibration failed\n\r");
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




//*****************************************************************************
//
// The callback function that is called by the touch screen driver to indicate
// activity on the touch screen.
//
//*****************************************************************************
static int32_t touchScreenCallback(uint32_t msg, int32_t x, int32_t y)
{
	int32_t swipeDiffX, swipeDiffY;
    if(app_ctx.swipe_enabled)
    {
        switch(msg)
        {
            // The user has just touched the screen.
            case WIDGET_MSG_PTR_DOWN:
            {
                // Save this press location.
            	swipe.swipeStarted = true;
            	if(!swipe.swipeOnGoing)
            	{
					swipe.initX = x;
					swipe.initY = y;

            	}
            	else
            	{	++swipe.sampleNum;
            		swipe.bufX[swipe.sampleNum % SWIPE_LAST_VAL_BUF_SIZE] = x;
            		swipe.bufY[swipe.sampleNum % SWIPE_LAST_VAL_BUF_SIZE] = y;
            	}

                break;
            }

            // The user has moved the touch location on the screen.
            case WIDGET_MSG_PTR_MOVE:
            {
            	if(swipe.swipeStarted || swipe.swipeOnGoing)
            	{
            		++swipe.sampleNum;
            		swipe.bufX[swipe.sampleNum % SWIPE_LAST_VAL_BUF_SIZE] = x;
            		swipe.bufY[swipe.sampleNum % SWIPE_LAST_VAL_BUF_SIZE] = y;
					swipe.swipeOnGoing = true;
            	}
                break;
            }

            // The user just stopped touching the screen.
            case WIDGET_MSG_PTR_UP:
            {
            	if(swipe.swipeOnGoing)
            	{
            		//checks on last gathered data for now
            		int32_t xLastVal = swipe.bufX[swipe.sampleNum % SWIPE_LAST_VAL_BUF_SIZE];
            		int32_t yLastVal = swipe.bufY[swipe.sampleNum % SWIPE_LAST_VAL_BUF_SIZE];
            		bool xLessThanInit = xLastVal < swipe.initX;
            		bool yLessThanInit = yLastVal < swipe.initY;
            		swipeDiffX = ((xLastVal - swipe.initX)>0) ? (xLastVal - swipe.initX) : (swipe.initX - xLastVal);
            		swipeDiffY = ((yLastVal - swipe.initY)>0) ? (yLastVal - swipe.initY) : (swipe.initY - yLastVal);
            		// checks which difference is bigger
            		if(swipeDiffX > swipeDiffY )
            		{

            			if(!xLessThanInit && (swipeDiffX > SWIPE_MIN_DIFFERENCE))
						{
							swipe.swipeDirecttion = SWIPE_RIGHT;
						}
						else if(xLessThanInit && (swipeDiffX > SWIPE_MIN_DIFFERENCE))
						{
							swipe.swipeDirecttion = SWIPE_LEFT;
						}
            		}
            		else
            		{
						if(!yLessThanInit && (swipeDiffY > SWIPE_MIN_DIFFERENCE))
						{
							swipe.swipeDirecttion = SWIPE_DOWN;
						}
						else if(yLessThanInit && (swipeDiffY > SWIPE_MIN_DIFFERENCE))
						{
							swipe.swipeDirecttion = SWIPE_UP;
						}
            		}
            	}
        		swipe.swipeOnGoing = false;
        		swipe.swipeStarted = false;
        		swipe.sampleNum = 0;
        		break;
            }
            default:
        		swipe.swipeOnGoing = false;
        		swipe.swipeStarted = false;
        		swipe.sampleNum = 0;
        		break;
        }
    }
    WidgetPointerMessage(msg, x, y);
    return 0;
}


static void handleMovement(void)
{
	uint16_t newScreenIdx = uiGetCurrentScreen();
	if(app_ctx.swipe_enabled)
	{
		if(swipe.swipeDirecttion != SWIPE_NONE )
		{

			if(swipe.swipeDirecttion == SWIPE_RIGHT)
			{
			    newScreenIdx = uiGetCurrentScreenContainer()->right;
			}
			else if(swipe.swipeDirecttion == SWIPE_LEFT)
			{
			    newScreenIdx = uiGetCurrentScreenContainer()->left;
			}
			else if(swipe.swipeDirecttion == SWIPE_UP)
			{
			    newScreenIdx = uiGetCurrentScreenContainer()->up;
			}
			else if(swipe.swipeDirecttion == SWIPE_DOWN)
			{
			    newScreenIdx = uiGetCurrentScreenContainer()->down;
			}
		}
		if(newScreenIdx != uiGetCurrentScreen())
		{
            WidgetRemove(uiGetCurrentScreenContainer()->widget);
            uiSetCurrentScreen(newScreenIdx);
            WidgetAdd(WIDGET_ROOT, uiGetCurrentScreenContainer()->widget);
            WidgetPaint(WIDGET_ROOT);
            get_new_temp_data = false;  //clean flag
            global_counter_sec = 1; 	  //clean counter
            WidgetMessageQueueProcess();
            uiUpdateScreen();
		}
	}
	swipe.swipeDirecttion = SWIPE_NONE;
}




//*****************************************************************************
//
// The interrupt handler for the for Systick interrupt.
// global_counter_sec updated 5 times per second
//
//*****************************************************************************
void SysTickIntHandler(void)
{
	static volatile uint32_t global_counter_sec = 0;
	global_counter_sec++;
	if((global_counter_sec % 100) == 0 && !get_new_temp_data) //every 20s
	{
		get_new_temp_data = true;
	}
}



// Main method of the application
int main(void)
{
	static uint32_t touch_screen_pressed_time = 0;

	//FPU
	MAP_FPUEnable();
	MAP_FPULazyStackingEnable();

	// Setup the system clock to run at 80 MHz from PLL with crystal reference
	MAP_SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

	// Init watchdog
	watchdog_init();

	// Reads from non-volatile memory
	readConfigAndSetAppContext();

	// Debug Console
	debugConsoleInit();

	// Display driver
	ILI9320Init();

	// Read Configuration
	configInit();

	// UI
	uiScreenSettings_init(&app_ctx.eeproparams, &app_ctx.flash_params);
    uiInit();

    // spi bus init
    spiCommonInit();

    if (!RF22_init())
    {
    	while(1);
    }

	// touchScreenControler
	touchScreenInit();

    // Set the touch screen event handler.
	touchScreenSetTouchCallback(touchScreenCallback);

	// do touch screen calibration if needed
	setTouchScreenCalibration();

	app_ctx.flash_params.connectionSetupState.wifiEnabled = true; //FOR DEBUG TODO

	// Wifi client init

	wifiInit(app_ctx.eeproparams.wifi_config.ap_ssid,
			 app_ctx.eeproparams.wifi_config.ap_wpa2_pass);
	wifiCheckApConnectionStatus();

	if (wifiGetConnectionStatus() != WIFI_CONNECTED)
	{
		if (app_ctx.flash_params.connectionSetupState.wifiEnabled)
		{
			wifiConnectToAp(); //try to connect
		}
	}

	// time module initialization
	timeInit(wifiFetchCurrentNtpTime);
	timeSetTimeZone(timeZoneCEST);

	// Enable the SysTick and its Interrupt.
	MAP_SysTickPeriodSet(MAP_SysCtlClockGet()); //0.2[s];
	MAP_SysTickIntEnable();
	MAP_SysTickEnable();

	// Enable all interrupts
	ENABLE_ALL_INTERRUPTS();
	get_new_temp_data = true; //update all for first time

	while (1)
	{
        // Process any messages in the widget message queue.
        WidgetMessageQueueProcess();

        // handle swipe moves on the screen
        handleMovement();

#if 0 //paint all places where finger touched
		if(!ADS7843getIntPinState()) //if touch panel is being touched
		{
			ADS7843read();
			TouchPoint a;
			a = ADS7843getTouchedPoint();
			DEBUG(MAIN_DEBUG_ENABLE, name, "RESULTS: x=%d, y=%d\n\r", a.x, a.y);
			GrContextForegroundSet(&drawing_context, ClrRed);
			GrCircleFill(&drawing_context, a.x, a.y, 3);
		}

#endif

		if (get_new_temp_data)
		{
			if (app_ctx.flash_params.connectionSetupState.wifiEnabled)
			{
				if (uiGetCurrentScreen() == SCREEN_MAIN)
				{
					uint8_t uiUpdateCnt = 0;
					if (!wifiFetchCurrentWeather(app_ctx.eeproparams.city_name))
					{
						DEBUG(MAIN_DEBUG_ENABLE, name, "wifiFetchCurrentWeather failed\n\r");
					}
					else
					{
						uiUpdateCnt++;
					}
					if (timeIsTimeToBeUpdated())
					{
						if (timeUpdateNow())
						{
							uiUpdateCnt++;
						}
					}
					if (uiUpdateCnt != 0)
					{
						uiUpdateScreen();
					}
				}
			}
			get_new_temp_data = false;
		}

		if(!ADS7843getIntPinState()) // if touch panel is being touched)
		{
			if(touch_screen_pressed_time == 0) // first press
			{
				touch_screen_pressed_time = global_counter_sec;
			}
			if((global_counter_sec - touch_screen_pressed_time) > 50) // if > ~10s do screen calibration
			{
				DISABLE_ALL_INTERRUPTS();
				delay_ms(1500);
				configEepromSetModified(&(app_ctx.eeproparams));
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
		watchdog_reset();
	}
}




