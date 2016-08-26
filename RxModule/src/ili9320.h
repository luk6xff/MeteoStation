/*
 * ili9320.h
 *
 *  Created on: 25-08-2016
 *      Author: lukasz
 */

#ifndef ILI9320_H_
#define ILI9320_H_



#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "em_device.h"
#include "em_bitband.h"
#include "em_gpio.h"
#include "em_cmu.h"


#define _8_BIT_MODEx
#define _16_BIT_MODE     //default
#ifdef _8_BIT_MODE
#undef _16_BIT_MODE
#endif

//Basic Colors
#define RED         0xf800
#define GREEN		0x07e0
#define BLUE		0x001f
#define BLACK		0x0000
#define YELLOW		0xffe0
#define WHITE		0xffff

//Other Colors
#define CYAN		0x07ff
#define BRIGHT_RED	0xf810
#define GRAY1		0x8410
#define GRAY2		0x4208

static const uint16_t colors[] = { 0xf800, 0x07e0, 0x001f, 0x0000, 0xffe0, 0xffff,
		0x07ff, 0xf810, 0x8410, 0x4208 };
//TFT resolution 240*320
#define MIN_X	    0
#define MIN_Y	    0
#define MAX_X	    239
#define MAX_Y	    319


#define PORTRAIT 0
#define LANDSCAPE 1
#define LEFT 0
#define RIGHT 9999
#define CENTER 9998

#define fontbyte(x) (cfont.font[x])

struct CurrentFont {
	uint8_t* font;
	uint8_t xSize;
	uint8_t ySize;
	uint8_t offset;
	uint8_t numchars;
};



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
void ILI9320printNumF(double num, uint8_t dec, int x, int y, char divider, int length,
		char filler, uint16_t color);
void ILI9320setXY(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void ILI9320clrXY(void);

void convertFloat(char *buf, double num, int width, uint8_t prec);

#endif /* ILI9320_H_ */
