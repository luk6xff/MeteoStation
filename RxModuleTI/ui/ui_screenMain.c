/*
 * ui_screenMain.c
 *
 *  Created on: 2 lip 2017
 *      Author: igbt6
 */

#include "ui_common.h"


//modules
#include "../time_lib.h"
#include "../wifi.h"

//Main screen widgets
/*
 * @brief data buffers
 */
static char ui_timeBuf[30];
static char ui_tempBuf[30];
static char ui_pressureBuf[30];
static char ui_humidityBuf[30];
static char ui_statusBuf[30];
static char ui_cityBuf[30];


/*
 * @brief UI structures
 */
//Time
Canvas(
		ui_timeCanvas,
		&ui_screenMainBackground,
		0,
		0,
		&g_ILI9320,
		20,
		25,
		280,
		20,
		CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_HCENTER |
		CANVAS_STYLE_TEXT_TOP | CANVAS_STYLE_TEXT_OPAQUE,
		BG_COLOR_MAIN,
		ClrWhite,
		ClrWhite,
		g_psFontCmss20,
		ui_timeBuf,
		0,
		0
);

//Temperature
Canvas(
		ui_tempCanvas,
		&ui_screenMainBackground,
		&ui_timeCanvas,
		0,
		&g_ILI9320,
		20,
		175,
		100,
		50,
		CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_RIGHT |
		CANVAS_STYLE_TEXT_TOP | CANVAS_STYLE_TEXT_OPAQUE,
		BG_COLOR_MAIN,
		ClrWhite,
		ClrWhite,
		g_psFontCmss48,
		ui_tempBuf,
		0,
		0
);

//Pressure Canvas
Canvas(
		ui_pressureCanvas,
		&ui_screenMainBackground,
		&ui_tempCanvas,
		0,
		&g_ILI9320,
		20,
		140,
		160,
		25,
		CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_LEFT | CANVAS_STYLE_TEXT_OPAQUE,
		BG_COLOR_MAIN,
		ClrWhite,
		ClrWhite,
		g_psFontCmss20,
		ui_pressureBuf,
		0,
		0
);

//Humidity
Canvas(
		ui_humidityCanvas,
		&ui_screenMainBackground,
		&ui_pressureCanvas,
		0,
		&g_ILI9320,
		20,
		105,
		160,
		25,
		CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_LEFT |
		CANVAS_STYLE_TEXT_OPAQUE,
		BG_COLOR_MAIN,
		ClrWhite,
		ClrWhite,
		g_psFontCmss20,
		ui_humidityBuf,
		0,
		0
);


//Status
Canvas(
		ui_statusCanvas,
		&ui_screenMainBackground,
		&ui_humidityCanvas, 0,
		&g_ILI9320,
		20,
		110,
		160,
		25,
		CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_LEFT | CANVAS_STYLE_TEXT_OPAQUE,
		BG_COLOR_MAIN,
		ClrWhite,
		ClrWhite,
		g_psFontCmss20,
		ui_statusBuf,
		0,
		0
);

//City - TODO
Canvas(
		ui_cityCanvas,
		&ui_screenMainBackground,
		&ui_statusCanvas,
		0,
		&g_ILI9320,
		20,
		40,
		240,
		25,
		CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_LEFT |
		CANVAS_STYLE_TEXT_OPAQUE,
		BG_COLOR_MAIN,
		ClrWhite,
		ClrWhite,
		g_psFontCmss20,
		ui_cityBuf,
		0,
		0
);

//Main
Canvas(
		ui_screenMainBackground,
		WIDGET_ROOT,
		0,
		&ui_cityCanvas,
		&g_ILI9320,
		BG_MIN_X,
		BG_MIN_Y,
		BG_MAX_X - BG_MIN_X,
		BG_MAX_Y - BG_MIN_Y,
		CANVAS_STYLE_FILL,
		BG_COLOR_MAIN,
		ClrWhite,
		ClrBlueViolet,
		g_psFontCmss20,
		0,
		0,
		0
);


//
//@brief helper method updates Clock UI if needed
//
static void uiUpdateClock()
{
	if (uiGetCurrentScreen() == SCREEN_MAIN)
	{
		if (timeNow() != 0)
		{
			int year = yearNow(); //update cache
			sprintf(ui_timeBuf, "%d-%02d-%02d  %02d:%02d", year, timeCurrentData().Month, timeCurrentData().Day, timeCurrentData().Hour, timeCurrentData().Minute);
		}
		WidgetPaint((tWidget*)&ui_timeCanvas);
	}
}



void uiScreenMainUpdate(void)
{
	if (uiGetCurrentScreen() != SCREEN_MAIN)
	{
		return;
	}
	tContext* drawingCtx = uiGetMainDrawingContext();
	uiClearBackground(); //clear images
	WifiWeatherDataModel data = wifiGetWeatherResultData();
	if (data.is_valid == 0)
	{
		sprintf(ui_humidityBuf, "Humidity: %d %s", data.humidity, "%");
		sprintf(ui_pressureBuf, "Pressure: %d hPa", data.pressure);
		sprintf(ui_tempBuf,"%d C", data.temperature);

		if(timeNow() > data.sunrise_time && timeNow() < data.sunset_time) //day
		{
			GrTransparentImageDraw(drawingCtx, img_sun, 185, 80, 0);
		}
		else //night
		{
			GrTransparentImageDraw(drawingCtx, img_moon, 185, 80, 0);
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
				GrTransparentImageDraw(drawingCtx, img_thunderStorm, 185, 80, 0);
			}
			//rain
			else if ((data.weather_cond_code[i] >= 300 && data.weather_cond_code[i] < 400) &&
					(data.weather_cond_code[i] >= 500 && data.weather_cond_code[i] < 500))
			{
				GrTransparentImageDraw(drawingCtx, img_rain, 185, 80, 0);
			}
			//snow
			else if (data.weather_cond_code[i] >= 600 && data.weather_cond_code[i] < 700)
			{
				GrTransparentImageDraw(drawingCtx, img_snow, 185, 80, 0);
			}
			//clouds
			else if (data.weather_cond_code[i] >= 700 && data.weather_cond_code[i] < 1000 &&
					 data.weather_cond_code[i] != 800)
			{
				GrTransparentImageDraw(drawingCtx, img_cloudy, 185, 80, 0);
			}
		}
	}
	else
	{
		sprintf(ui_humidityBuf, "Humidity: -- %s", "%");
		sprintf(ui_pressureBuf, "Pressure: --- hPa");
		sprintf(ui_tempBuf,"--- C");
	}
	WidgetPaint((tWidget*)&ui_humidityCanvas);
	WidgetPaint((tWidget*)&ui_pressureCanvas);
	WidgetPaint((tWidget*)&ui_tempCanvas);
	uiUpdateClock();
}
