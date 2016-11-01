/*
 * ILI9320.h
 *
 *  Created on: 30-08-2016
 *      Author: igbt6
 */

#ifndef ILI9320_H_
#define ILI9320_H_

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif



#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define LEFT 0
#define RIGHT 9999
#define CENTER 9998

static const uint16_t DISPLAY_WIDTH_X = 240;
static const uint16_t DISPLAY_HEIGHT_Y = 320;

typedef struct {
	uint8_t* font;
	uint8_t xSize;
	uint8_t ySize;
	uint8_t offset;
	uint8_t numchars;
}Font;

typedef struct {
	uint16_t x;
	uint16_t y;
	uint16_t maxX;
	uint16_t maxY;
}Resolution;

typedef struct {
	uint16_t x;
	uint16_t y;
}DispalySize;

typedef enum {
	BLACK = 0x0000,
	WHITE = 0xFFFF,
	RED = 0xF800,
	BRIGHT_RED = 0xF810,
	GREEN = 0x0400,
	LIGHT_GREEN = 0x07E0,
	BLUE = 0x001F,
	SILVER = 0xC618,
	GRAY = 0x8410,
	MAROON = 0x8000,
	YELLOW = 0xFFE0,
	OLIVE = 0x8400,
	LIME = 0x07E0,
	CYAN = 0x07FF,
	TEAL = 0x0410,
	NAVY = 0x0010,
	FUCHSIA = 0xF81F,
	PURPLE = 0x8010,
	TRANSPARENT = 0xFFFF
}Colors;

typedef enum {
	_8_BIT, _16_BIT
}TransferMode;

typedef enum {
	PORTRAIT, LANDSCAPE
}Orientation;


uint16_t ReadRegister(uint16_t index);

void init(TransferMode tMode, Orientation o);
void clrScr();
void drawPixel(int x, int y, uint16_t color);
void drawLine(int x1, int y1, int x2, int y2, uint16_t color);
void fillScreenBackground(uint16_t color);
void drawRect(int x1, int y1, int x2, int y2, uint16_t color);
void drawRoundRect(int x1, int y1, int x2, int y2, uint16_t color);
void fillRect(int x1, int y1, int x2, int y2, uint16_t color);
void fillRoundRect(int x1, int y1, int x2, int y2, uint16_t color);
void drawCircle(int x, int y, int radius, uint16_t color);
void fillCircle(int x, int y, int radius, uint16_t color);
void setFont(const uint8_t* font);
//void setFont(const Font* font);
uint8_t* getFont();
uint8_t getFontXsize();
uint8_t getFontYsize();
void drawBitmap(int x, int y, int sx, int sy, const uint16_t* bitmap,
		int scale);
void lcdOff();
void lcdOn();
void setContrast(char c);
void lcdWriteCOMMAND(uint16_t data);
void lcdWriteDATA(uint16_t data);
void lcdWriteCOMMAND_DATA(uint16_t com1, uint16_t dat1);
void setPixel(uint16_t color);
void drawHLine(int x, int y, int l, uint16_t color);
void drawVLine(int x, int y, int l, uint16_t color);
void printChar(uint8_t c, int x, int y, uint16_t color);
void rotateChar(uint8_t c, int x, int y, int pos, int deg,
		uint16_t color);
void print(char *st, int x, int y, int deg, uint16_t color);
void printNumI(long num, int x, int y, int length, char filler,
		uint16_t color);
void printNumF(double num, uint8_t dec, int x, int y, char divider,
		int length, char filler, uint16_t color);
void setXY(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void clrXY(void);
void convertFloat(char *buf, double num, int width, uint8_t prec);
//calibration helpers
void showThreePointCalibrationHitPoint(uint16_t x1, uint16_t y1);
void setColor(uint8_t r, uint8_t g, uint8_t b);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif /* ILI9320_H_ */
