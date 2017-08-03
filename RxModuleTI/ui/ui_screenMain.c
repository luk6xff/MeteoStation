/*
 * ui_screenMain.c
 *
 *  Created on: 2 lip 2017
 *      Author: igbt6
 */

#include "ui_common.h"

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
