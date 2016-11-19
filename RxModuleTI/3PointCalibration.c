/*
 * 3PointCalibration.c
 *
 *  Created on: 18 lis 2016
 *      Author: igbt6
 */
#include "3PointCalibration.h"
#include "delay.h"

uint8_t performThreePointCalibration(tContext* ctx, CalibCoefficients* coefs)
{

	const uint16_t delayMs = 2000; //2s
	TouchPoint p1, p2, p3;
	TouchPoint t1, t2, t3;
	// point 1 is at 25%,50%, 2 is at 75%,25% and 3 is at 75%,75%
	uint16_t size_y = ctx->psDisplay->ui16Height;
	uint16_t size_x = ctx->psDisplay->ui16Width;
	p1.x = (size_x * 25) / 100;
	p1.y = (size_y * 50) / 100;
	p2.x = (size_x * 75) / 100;
	p2.y = (size_y * 25) / 100;
	p3.x = (size_x * 75) / 100;
	p3.y = (size_y * 75) / 100;
	//ADS7843_INT_IRQ_CONFIG_FALLING(false); //disable ext interrupts //TODO
	//ADS7843_INT_IRQ_CONFIG_RISING(false); //TODO
	//1st point
    GrContextForegroundSet(ctx, ClrLightGoldenrodYellow);
    GrContextFontSet(ctx, &g_sFontCm16);
    GrStringDrawCentered(ctx, "Tap & hold the point", -1,
                         GrContextDpyWidthGet(ctx) / 2, 8, 0);

    GrContextForegroundSet(ctx, ClrBlue);
	drawCalibrationPoint(ctx, p1.x, p1.y, 30);
	ADS7843readOnePointRawCoordinates(&t1);
    GrContextForegroundSet(ctx, ClrYellow);
	drawCalibrationPoint(ctx, p1.x, p1.y, 30);
	delay_ms(delayMs);
	//2nd point
    GrContextForegroundSet(ctx, ClrBlue);
	drawCalibrationPoint(ctx, p2.x, p2.y, 30);
	ADS7843readOnePointRawCoordinates(&t2);
    GrContextForegroundSet(ctx, ClrYellow);
	drawCalibrationPoint(ctx, p2.x, p2.y, 30);
	delay_ms(delayMs);
	//3rd point
    GrContextForegroundSet(ctx, ClrBlue);
	drawCalibrationPoint(ctx, p3.x, p3.y, 30);
	ADS7843readOnePointRawCoordinates(&t3);
    GrContextForegroundSet(ctx, ClrYellow);
	drawCalibrationPoint(ctx, p3.x, p3.y, 30);
	delay_ms(delayMs);

	//final computation based on:
	//https://www.maximintegrated.com/en/app-notes/index.mvp/id/5296
	// and
	//http://www.ti.com/lit/an/slyt277/slyt277.pdf
	int32_t delta, deltaX1, deltaX2, deltaX3, deltaY1, deltaY2, deltaY3;
	// intermediate values for the calculation
	delta = ((int32_t) (t1.x - t3.x) * (int32_t) (t2.y - t3.y))
			- ((int32_t) (t2.x - t3.x) * (int32_t) (t1.y - t3.y));

	deltaX1 = ((int32_t) (p1.x - p3.x) * (int32_t) (t2.y - t3.y))
			- ((int32_t) (p2.x - p3.x) * (int32_t) (t1.y - t3.y));
	deltaX2 = ((int32_t) (t1.x - t3.x) * (int32_t) (p2.x - p3.x))
			- ((int32_t) (t2.x - t3.x) * (int32_t) (p1.x - p3.x));
	deltaX3 =
			p1.x
					* ((int32_t) t2.x * (int32_t) t3.y
							- (int32_t) t3.x * (int32_t) t2.y)
					- p2.x
							* (t1.x * (int32_t) t3.y
									- (int32_t) t3.x * (int32_t) t1.y)
					+ p3.x
							* ((int32_t) t1.x * (int32_t) t2.y
									- (int32_t) t2.x * (int32_t) t1.y);

	deltaY1 = ((int32_t) (p1.y - p3.y) * (int32_t) (t2.y - t3.y))
			- ((int32_t) (p2.y - p3.y) * (int32_t) (t1.y - t3.y));
	deltaY2 = ((int32_t) (t1.x - t3.x) * (int32_t) (p2.y - p3.y))
			- ((int32_t) (t2.x - t3.x) * (int32_t) (p1.y - p3.y));
	deltaY3 =
			p1.y
					* ((int32_t) t2.x * (int32_t) t3.y
							- (int32_t) t3.x * (int32_t) t2.y)
					- p2.y
							* ((int32_t) t1.x * (int32_t) t3.y
									- (int32_t) t3.x * (int32_t) t1.y)
					+ p3.y
							* ((int32_t) t1.x * (int32_t) t2.y
									- (int32_t) t2.x * t1.y);

	// final values
	coefs->m_ax = (float) deltaX1 / (float) delta;
	coefs->m_bx = (float) deltaX2 / (float) delta;
	coefs->m_dx = (float) deltaX3 / (float) delta;

	coefs->m_ay = (float) deltaY1 / (float) delta;
	coefs->m_by = (float) deltaY2 / (float) delta;
	coefs->m_dy = (float) deltaY3 / (float) delta;
    GrContextForegroundSet(ctx, ClrRed);
    GrContextFontSet(ctx, &g_sFontCm16);
    GrStringDrawCentered(ctx, "Success-storing data...", -1,
                         GrContextDpyWidthGet(ctx) / 2, 50, 0);
	delay_ms(delayMs);
	ctx->psDisplay->pfnFlush((void *)0);
	//ADS7843_INT_IRQ_CONFIG_FALLING(true); //TODO

	return 0;
}

void drawCalibrationPoint(const tContext* ctx, uint16_t x, uint16_t y,
		uint16_t radius) {
	GrCircleDraw(ctx, x, y, radius);
	GrCircleFill(ctx, x, y, radius - 20);
}
