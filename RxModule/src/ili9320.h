/*
 * ILI9320.h
 *
 *  Created on: 30-08-2016
 *      Author: igbt6
 */



#ifndef ILI9320_H_
#define ILI9320_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#define LEFT 0
#define RIGHT 9999
#define CENTER 9998


struct Font
{
	uint8_t* font;
	uint8_t xSize;
	uint8_t ySize;
	uint8_t offset;
	uint8_t numchars;
};

struct Resolution
{
	uint16_t x;
	uint16_t y;
	uint16_t maxX;
	uint16_t maxY;

	Resolution(uint16_t _x,uint16_t _y):x(_x),y(_y)
	{
		maxX = x-1;
		maxY = y-1;
	}
};

class ILI9320 {

public:

	enum Colors
	{
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
	};

	enum TransferMode
	{
		_8_BIT,
		_16_BIT
	};

	enum Orientation
	{
		PORTRAIT,
		LANDSCAPE
	};

public:

	ILI9320(TransferMode tMode= _16_BIT,Orientation o=PORTRAIT);
	virtual ~ILI9320();
	void ILI9320init(void);
	void ILI9320clrScr();
	void ILI9320drawPixel(int x, int y, uint16_t color);
	void ILI9320drawLine(int x1, int y1, int x2, int y2, uint16_t color);
	void ILI9320fillScreenBackground(uint16_t color);
	void ILI9320drawRect(int x1, int y1, int x2, int y2, uint16_t color);
	void ILI9320drawRoundRect(int x1, int y1, int x2, int y2, uint16_t color);
	void ILI9320fillRect(int x1, int y1, int x2, int y2, uint16_t color);
	void ILI9320fillRoundRect(int x1, int y1, int x2, int y2, uint16_t color);
	void ILI9320drawCircle(int x, int y, int radius, uint16_t color);
	void ILI9320fillCircle(int x, int y, int radius, uint16_t color);
	void ILI9320setFont(const uint8_t* font);
	//void ILI9320setFont(const Font* font);
	uint8_t* ILI9320getFont();
	uint8_t ILI9320getFontXsize();
	uint8_t ILI9320getFontYsize();
	void ILI9320drawBitmap(int x, int y, int sx, int sy, const uint16_t* bitmap, int scale);
	void ILI9320lcdOff();
	void ILI9320lcdOn();
	void ILI9320setContrast(char c);
	void ILI9320lcdWriteCOMMAND(uint16_t data);
	void ILI9320lcdWriteDATA(uint16_t data);
	void ILI9320lcdWriteCOMMAND_DATA(uint16_t com1, uint16_t dat1);
	void ILI9320setPixel(uint16_t color);
	void ILI9320drawHLine(int x, int y, int l, uint16_t color);
	void ILI9320drawVLine(int x, int y, int l, uint16_t color);
	void ILI9320printChar(uint8_t c, int x, int y, uint16_t color);
	void ILI9320rotateChar(uint8_t c, int x, int y, int pos, int deg,uint16_t color);
	void ILI9320print(char *st, int x, int y, int deg, uint16_t color);
	void ILI9320printNumI(long num, int x, int y, int length, char filler, uint16_t color);
	void ILI9320printNumF(double num, uint8_t dec, int x, int y, char divider, int length, char filler, uint16_t color);
	void ILI9320setXY(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
	void ILI9320clrXY(void);

	void ILI9320convertFloat(char *buf, double num, int width, uint8_t prec);

private:
	TransferMode mTransferMode;
	Orientation mOrientation;
	Resolution mResolution;
	Font mCurrentFont;
};

#endif /* ILI9320_H_ */
