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
#include "system.h"
#include "time_lib.h"
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
		"Disconnect"
		"Connect",
};


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
	ConfigEepromParameters eeprom_params;
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



// period notifiers
// @brief new weather data from wifi
static volatile bool m_get_new_temp_data = false;
// @brief global counter
static volatile uint32_t m_global_counter_sec = 0;



//*****************************************************************************
//
// Methods forward declarations.
//
//*****************************************************************************
static int32_t touchScreenCallback(uint32_t msg, int32_t x, int32_t y);
static void ui_updateScreen();
static void ui_updateClock(bool drawOnly);

//*****************************************************************************
//
// Private methods
//
//*****************************************************************************
static bool saveApplicationContextToMemory()
{
	DISABLE_ALL_INTERRUPTS();
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
	ENABLE_ALL_INTERRUPTS();
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

	//configFlashSetInvalid(configFlashGetCurrent());  //new memory layout introduced - comment it out later
	if(configFlashIsInvalid(configFlashGetCurrent()))
	{
		configFlashSaveDefaults();
	}
	else
	{
		// assign defaults settings from memory
		memcpy(&m_app_ctx.flash_params, configFlashGetCurrent(), sizeof(ConfigFlashParameters));
	}

	//configEepromSetInvalid(configEepromGetCurrent()); //new memory layout introduced - comment it out later
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





//*****************************************************************************
//
// @brief Update UI screens with settings, data and other values
//
//*****************************************************************************
static void ui_updateScreen()
{
    switch (m_app_ctx.current_screen)
    {
    	case SCREEN_MAIN:
    	{
    		uiClearBackground(); //clear images
    		WifiWeatherDataModel data = wifiGetWeatherResultData();
    		if (data.is_valid == 0)
    		{
    			sprintf(ui_humidityBuf, "Humidity: %d %s", data.humidity, "%");
    			sprintf(ui_pressureBuf, "Pressure: %d hPa", data.pressure);
    			sprintf(ui_tempBuf,"%d C", data.temperature);

    			if(timeNow() > data.sunrise_time && timeNow() < data.sunset_time) //day
    			{
    				GrTransparentImageDraw(&m_drawing_context, img_sun, 185, 80, 0);
    			}
    			else //night
    			{
    				GrTransparentImageDraw(&m_drawing_context, img_moon, 185, 80, 0);
    			}
    			// convert codes weather conditions codes to images as described at:
    			// https://openweathermap.org/weather-conditions
    			for (size_t i = 0; i < 3; ++i)
    			{
    				if (data.weather_cond_code[i] == -1)
    				{
    					continue;
    				}
    				//thunder storm
    				if (data.weather_cond_code[i] >= 200 && data.weather_cond_code[i] < 300)
    				{
    					GrTransparentImageDraw(&m_drawing_context, img_thunderStorm, 185, 80, 0);
    				}
    				//rain
    				else if ((data.weather_cond_code[i] >= 300 && data.weather_cond_code[i] < 400) &&
    						(data.weather_cond_code[i] >= 500 && data.weather_cond_code[i] < 500))
    				{
    					GrTransparentImageDraw(&m_drawing_context, img_rain, 185, 80, 0);
    				}
    				//snow
    				else if (data.weather_cond_code[i] >= 600 && data.weather_cond_code[i] < 700)
    				{
    					GrTransparentImageDraw(&m_drawing_context, img_snow, 185, 80, 0);
    				}
    				//clouds
    				else if (data.weather_cond_code[i] >= 700 && data.weather_cond_code[i] < 1000 &&
    						 data.weather_cond_code[i] != 800)
    				{
    					GrTransparentImageDraw(&m_drawing_context, img_cloudy, 185, 80, 0);
    				}
    			}
    		}
    		else
    		{
    			sprintf(ui_humidityBuf, "Humidity: -- %s", "%");
    			sprintf(ui_pressureBuf, "Pressure: --- hPa");
    			sprintf(ui_tempBuf,"--- C");
    		}
    		ui_updateClock(true);
    		WidgetPaint((tWidget*)&ui_humidityCanvas);
    		WidgetPaint((tWidget*)&ui_pressureCanvas);
    		WidgetPaint((tWidget*)&ui_tempCanvas);
    		break;
    	}
    	case SCREEN_CONN_SETTINGS:
    	{
    		if (m_app_ctx.flash_params.connectionSetupState.wifiEnabled)
    		{
    			CheckBoxSelectedOn(&ui_settingsCheckBoxes[0]);
    		    CanvasImageSet(&ui_settingsCheckBoxIndicators[0], img_lightOn);
    		}
    		else
    		{
    			CheckBoxSelectedOff(&ui_settingsCheckBoxes[0]);
    		    CanvasImageSet(&ui_settingsCheckBoxIndicators[0], img_lightOff);
    		}

    		if (m_app_ctx.flash_params.connectionSetupState.sensorsEnabled)
    		{
    			CheckBoxSelectedOn(&ui_settingsCheckBoxes[1]);
    		    CanvasImageSet(&ui_settingsCheckBoxIndicators[1], img_lightOn);
    		}
    		else
    		{
    			CheckBoxSelectedOff(&ui_settingsCheckBoxes[1]);
    		    CanvasImageSet(&ui_settingsCheckBoxIndicators[1], img_lightOff);
    		}

    		if (m_app_ctx.flash_params.connectionSetupState.powerSavingEnabled)
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
					GrContextFontSet(&m_drawing_context, &g_sFontCm12);
					GrContextForegroundSet(&m_drawing_context, ClrWhite);
					GrStringDrawCentered(&m_drawing_context, connStateDesc[WIFI_NOT_CONNECTED], -1, 250, 130, true);
					break;
				case WIFI_CONNECTED:
				case WIFI_TRANSMISSION_CREATED:
				case WIFI_TRANSMISSION_ENDED:
					GrContextFontSet(&m_drawing_context, &g_sFontCm16);
					GrContextForegroundSet(&m_drawing_context, ClrWhite);
					GrStringDrawCentered(&m_drawing_context, connStateDesc[WIFI_CONNECTED], -1, 250, 130, true);
					break;
				default:
					break;
			}
    		break;
    	}
    	default:
    		break;
    }
}

static void ui_updateClock(bool drawOnly)
{
	if (m_app_ctx.current_screen == SCREEN_MAIN)
	{
		if (timeIsTimeChanged() || drawOnly)
		{
			if (!drawOnly)
			{
				sprintf(ui_timeBuf, "%d-%02d-%02d  %02d:%02d", yearNow(), monthNow(), dayNow(), hourNow(), minuteNow());
			}
			WidgetPaint((tWidget*)&ui_timeCanvas);
		}

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
            		m_swipe.bufX[m_swipe.sampleNum % SWIPE_LAST_VAL_BUF_SIZE] = x;
            		m_swipe.bufY[m_swipe.sampleNum % SWIPE_LAST_VAL_BUF_SIZE] = y;
            	}

                break;
            }

            // The user has moved the touch location on the screen.
            case WIDGET_MSG_PTR_MOVE:
            {
            	if(m_swipe.swipeStarted || m_swipe.swipeOnGoing)
            	{
            		++m_swipe.sampleNum;
            		m_swipe.bufX[m_swipe.sampleNum % SWIPE_LAST_VAL_BUF_SIZE] = x;
            		m_swipe.bufY[m_swipe.sampleNum % SWIPE_LAST_VAL_BUF_SIZE] = y;
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
            		int32_t xLastVal = m_swipe.bufX[m_swipe.sampleNum % SWIPE_LAST_VAL_BUF_SIZE];
            		int32_t yLastVal = m_swipe.bufY[m_swipe.sampleNum % SWIPE_LAST_VAL_BUF_SIZE];
            		bool xLessThanInit = xLastVal < m_swipe.initX;
            		bool yLessThanInit = yLastVal < m_swipe.initY;
            		swipeDiffX = ((xLastVal - m_swipe.initX)>0) ? (xLastVal - m_swipe.initX) : (m_swipe.initX - xLastVal);
            		swipeDiffY = ((yLastVal - m_swipe.initY)>0) ? (yLastVal - m_swipe.initY) : (m_swipe.initY - yLastVal);
            		// checks which difference is bigger
            		if(swipeDiffX > swipeDiffY )
            		{

            			if(!xLessThanInit && (swipeDiffX > SWIPE_MIN_DIFFERENCE))
						{
							m_swipe.swipeDirecttion = SWIPE_RIGHT;
						}
						else if(xLessThanInit && (swipeDiffX > SWIPE_MIN_DIFFERENCE))
						{
							m_swipe.swipeDirecttion = SWIPE_LEFT;
						}
            		}
            		else
            		{
						if(!yLessThanInit && (swipeDiffY > SWIPE_MIN_DIFFERENCE))
						{
							m_swipe.swipeDirecttion = SWIPE_DOWN;
						}
						else if(yLessThanInit && (swipeDiffY > SWIPE_MIN_DIFFERENCE))
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
            m_get_new_temp_data = false;  //clean flag
            m_global_counter_sec = 1; 	  //clean counter
            m_app_ctx.current_screen = newScreenIdx;
            WidgetMessageQueueProcess();
            ui_updateScreen();
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
void SysTickIntHandler(void)
{
	static volatile uint32_t m_global_counter_sec = 0;
	m_global_counter_sec++;
	if((m_global_counter_sec % 100) == 0 && !m_get_new_temp_data) //every 20s
	{
		m_get_new_temp_data = true;
	}
}



// Main method of the application
int main(void)
{
	static uint32_t touch_screen_pressed_time = 0;

	//FPU
	FPUEnable();
	FPULazyStackingEnable();

	// Setup the system clock to run at 80 MHz from PLL with crystal reference
	SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

	//Reads from non-volatile memory
	readConfigAndSetAppContext();

	//Debug Console
	debugConsoleInit();

	//Display driver
	ILI9320Init();

	//Read Configuration
	configInit();

	//UI
    GrContextInit(&m_drawing_context, &g_ILI9320);
    uiInit(&m_drawing_context);
    uiFrameDraw(&m_drawing_context, "Meteo Ubiad Stacja");
    WidgetAdd(WIDGET_ROOT, m_screens[m_app_ctx.current_screen].widget);
    WidgetPaint(WIDGET_ROOT);
    uiDrawInitInfo();

	//touchScreenControler
	touchScreenInit();

    // Set the touch screen event handler.
	touchScreenSetTouchCallback(touchScreenCallback);

	//do touch screen calibration if needed
	setTouchScreenCalibration();

	m_app_ctx.flash_params.connectionSetupState.wifiEnabled = true; //FOR DEBUG TODO
	//Wifi client init
	wifiInit(m_app_ctx.eeprom_params.wifi_config.ap_ssid,
			 m_app_ctx.eeprom_params.wifi_config.ap_wpa2_pass);
	wifiCheckApConnectionStatus();
	if (m_app_ctx.flash_params.connectionSetupState.wifiEnabled)
	{
		wifiConnectToAp(); //try to connect
	}

	//time module initialization
	timeInit(wifiFetchCurrentNtpTime);
	timeSetTimeZone(timeZoneCET);

	// Enable the SysTick and its Interrupt.
	SysTickPeriodSet(SysCtlClockGet()); //0.2[s];
	SysTickIntEnable();
	SysTickEnable();

	//Enable all interrupts
	ENABLE_ALL_INTERRUPTS();
	m_get_new_temp_data = true; //update all for first time

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
			//MAIN_DEBUG("RESULTS: x=%d, y=%d\n\r", a.x, a.y);
			GrContextForegroundSet(&m_drawing_context, ClrRed);
			GrCircleFill(&m_drawing_context, a.x, a.y, 3);
		}

#endif
		if (m_get_new_temp_data)
		{
			if (m_app_ctx.flash_params.connectionSetupState.wifiEnabled)
			{
				if (wifiCheckApConnectionStatus())
				{
					WifiConnectionState state = wifiGetConnectionStatus();
					if (m_app_ctx.flash_params.connectionSetupState.wifiConnectionState != state)
					{
						if (state == WIFI_NOT_CONNECTED || state == WIFI_CONNECTED)
						{
							m_app_ctx.flash_params.connectionSetupState.wifiConnectionState = state;
							configFlashSetModified(&m_app_ctx.flash_params);
							saveApplicationContextToMemory();
						}
					}
				}
				if (m_app_ctx.current_screen == SCREEN_MAIN)
				{
					if (!wifiFetchCurrentWeather(m_app_ctx.eeprom_params.city_names[m_app_ctx.flash_params.currentCity]))
					{
						MAIN_DEBUG("wifiGetCurrentWeather failed\n\r");
					}
				}
			}
			ui_updateScreen();
			m_get_new_temp_data = false;
		}

		ui_updateClock(false);

		if(!ADS7843getIntPinState()) // if touch panel is being touched)
		{
			if(touch_screen_pressed_time == 0) // first press
			{
				touch_screen_pressed_time = m_global_counter_sec;
			}
			if((m_global_counter_sec - touch_screen_pressed_time) > 50) // if > ~10s do screen calibration
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
