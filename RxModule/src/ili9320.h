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


#define PORTRAIT 1
#define LANDSCAPE 0
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

/*********************************Hardware dependent part*****************************************/
/*********************************Hardware dependent part*****************************************/

#define CLOCKS_ENABLE() CMU_ClockEnable(cmuClock_GPIO, true)  //add more if needed
//==================/CS=====================
#define CS_PIN      9
#define CS_PORT     gpioPortB
#define CS_OUTPUT() GPIO_PinModeSet(CS_PORT, CS_PIN, gpioModePushPull, 1)
#define CS_HIGH()   GPIO_PinOutSet(CS_PORT , CS_PIN )
#define CS_LOW()    GPIO_PinOutClear(CS_PORT , CS_PIN )
//------------------RS----------------------

#define RS_PIN      10
#define RS_PORT     gpioPortB
#define RS_OUTPUT() GPIO_PinModeSet(RS_PORT, RS_PIN, gpioModePushPull, 1)
#define RS_HIGH()   GPIO_PinOutSet(RS_PORT , RS_PIN )
#define RS_LOW()    GPIO_PinOutClear(RS_PORT , RS_PIN )

//------------------WR----------------------

#define WR_PIN      11
#define WR_PORT     gpioPortB
#define WR_OUTPUT() GPIO_PinModeSet(WR_PORT, WR_PIN, gpioModePushPull, 1)
#define WR_HIGH()   GPIO_PinOutSet(WR_PORT , WR_PIN )
#define WR_LOW()    GPIO_PinOutClear(WR_PORT , WR_PIN )
#define WR_RISING() {WR_HIGH(); WR_LOW(); }
//------------------RD---------------------

#define RD_PIN      12
#define RD_PORT     gpioPortB
#define RD_OUTPUT() GPIO_PinModeSet(RD_PORT, RD_PIN, gpioModePushPull, 1)
#define RD_HIGH()   GPIO_PinOutSet(RD_PORT , RD_PIN )
#define RD_LOW()    GPIO_PinOutClear(RD_PORT , RD_PIN )
#define RD_RISING() {RD_HIGH(); RD_LOW(); }

//------------------TRANSISTOR---------------------
#define LCD_TRANS_ENABLE_PORT gpioPortA
#define LCD_TRANS_ENABLE_PIN 0//0
#define LCD_TRANS_ENABLE_OUTPUT() GPIO_PinModeSet(LCD_TRANS_ENABLE_PORT,LCD_TRANS_ENABLE_PIN, gpioModePushPull, 0)
#define LCD_TRANS_ENABLE_INPUT()  GPIO_PinModeSet(LCD_TRANS_ENABLE_PORT,LCD_TRANS_ENABLE_PIN, gpioModeInput, 0)

//------------------DATA-------------------
#define TFT_PORT_D15 gpioPortD
#define TFT_PIN_D15 7
#define TFT_PIN_D15_OUTPUT() GPIO_PinModeSet(TFT_PORT_D15,TFT_PIN_D15, gpioModePushPull, 1)
#define TFT_PIN_D15_INPUT()  GPIO_PinModeSet(TFT_PORT_D15,TFT_PIN_D15, gpioModeInput, 0)

#define TFT_PORT_D14 gpioPortD
#define TFT_PIN_D14 6
#define TFT_PIN_D14_OUTPUT() GPIO_PinModeSet(TFT_PORT_D14,TFT_PIN_D14, gpioModePushPull, 1)
#define TFT_PIN_D14_INPUT()  GPIO_PinModeSet(TFT_PORT_D14,TFT_PIN_D14, gpioModeInput, 0)

#define TFT_PORT_D13 gpioPortD
#define TFT_PIN_D13 5
#define TFT_PIN_D13_OUTPUT() GPIO_PinModeSet(TFT_PORT_D13,TFT_PIN_D13, gpioModePushPull, 1)
#define TFT_PIN_D13_INPUT()  GPIO_PinModeSet(TFT_PORT_D13,TFT_PIN_D13, gpioModeInput, 0)

#define TFT_PORT_D12 gpioPortD
#define TFT_PIN_D12 4
#define TFT_PIN_D12_OUTPUT() GPIO_PinModeSet(TFT_PORT_D12,TFT_PIN_D12, gpioModePushPull, 1)
#define TFT_PIN_D12_INPUT()  GPIO_PinModeSet(TFT_PORT_D12,TFT_PIN_D12, gpioModeInput, 0)

#define TFT_PORT_D11 gpioPortD
#define TFT_PIN_D11 14
#define TFT_PIN_D11_OUTPUT() GPIO_PinModeSet(TFT_PORT_D11,TFT_PIN_D11, gpioModePushPull, 1)
#define TFT_PIN_D11_INPUT()  GPIO_PinModeSet(TFT_PORT_D11,TFT_PIN_D11, gpioModeInput, 0)

#define TFT_PORT_D10 gpioPortD
#define TFT_PIN_D10 13
#define TFT_PIN_D10_OUTPUT() GPIO_PinModeSet(TFT_PORT_D10,TFT_PIN_D10, gpioModePushPull, 1)
#define TFT_PIN_D10_INPUT()  GPIO_PinModeSet(TFT_PORT_D10,TFT_PIN_D10, gpioModeInput, 0)

#define TFT_PORT_D9 gpioPortD
#define TFT_PIN_D9  8
#define TFT_PIN_D9_OUTPUT() GPIO_PinModeSet(TFT_PORT_D9,TFT_PIN_D9, gpioModePushPull, 1)
#define TFT_PIN_D9_INPUT()  GPIO_PinModeSet(TFT_PORT_D9,TFT_PIN_D9, gpioModeInput, 0)

#define TFT_PORT_D8 gpioPortD
#define TFT_PIN_D8  15
#define TFT_PIN_D8_OUTPUT() GPIO_PinModeSet(TFT_PORT_D8,TFT_PIN_D8, gpioModePushPull, 1)
#define TFT_PIN_D8_INPUT()  GPIO_PinModeSet(TFT_PORT_D8,TFT_PIN_D8, gpioModeInput, 0)

#ifdef _16_BIT_MODE

#define TFT_PORT_D7 gpioPortC
#define TFT_PIN_D7 7
#define TFT_PIN_D7_OUTPUT() GPIO_PinModeSet(TFT_PORT_D7,TFT_PIN_D7, gpioModePushPull, 1)
#define TFT_PIN_D7_INPUT()  GPIO_PinModeSet(TFT_PORT_D7,TFT_PIN_D7, gpioModeInput, 0)

#define TFT_PORT_D6 gpioPortC
#define TFT_PIN_D6 6
#define TFT_PIN_D6_OUTPUT() GPIO_PinModeSet(TFT_PORT_D6,TFT_PIN_D6, gpioModePushPull, 1)
#define TFT_PIN_D6_INPUT()  GPIO_PinModeSet(TFT_PORT_D6,TFT_PIN_D6, gpioModeInput, 0)

#define TFT_PORT_D5 gpioPortA
#define TFT_PIN_D5 12
#define TFT_PIN_D5_OUTPUT() GPIO_PinModeSet(TFT_PORT_D5,TFT_PIN_D5, gpioModePushPull, 1)
#define TFT_PIN_D5_INPUT()  GPIO_PinModeSet(TFT_PORT_D5,TFT_PIN_D5, gpioModeInput, 0)

#define TFT_PORT_D4 gpioPortA
#define TFT_PIN_D4 13
#define TFT_PIN_D4_OUTPUT() GPIO_PinModeSet(TFT_PORT_D4,TFT_PIN_D4, gpioModePushPull, 1)
#define TFT_PIN_D4_INPUT()  GPIO_PinModeSet(TFT_PORT_D4,TFT_PIN_D4, gpioModeInput, 0)

#define TFT_PORT_D3 gpioPortF
#define TFT_PIN_D3 8
#define TFT_PIN_D3_OUTPUT() GPIO_PinModeSet(TFT_PORT_D3,TFT_PIN_D3, gpioModePushPull, 1)
#define TFT_PIN_D3_INPUT()  GPIO_PinModeSet(TFT_PORT_D3,TFT_PIN_D3, gpioModeInput, 0)

#define TFT_PORT_D2 gpioPortF
#define TFT_PIN_D2 9
#define TFT_PIN_D2_OUTPUT() GPIO_PinModeSet(TFT_PORT_D2,TFT_PIN_D2, gpioModePushPull, 1)
#define TFT_PIN_D2_INPUT()  GPIO_PinModeSet(TFT_PORT_D2,TFT_PIN_D2, gpioModeInput, 0)

#define TFT_PORT_D1 gpioPortC
#define TFT_PIN_D1  1
#define TFT_PIN_D1_OUTPUT() GPIO_PinModeSet(TFT_PORT_D1,TFT_PIN_D1, gpioModePushPull, 1)
#define TFT_PIN_D1_INPUT()  GPIO_PinModeSet(TFT_PORT_D1,TFT_PIN_D1, gpioModeInput, 0)

#define TFT_PORT_D0 gpioPortC
#define TFT_PIN_D0 0
#define TFT_PIN_D0_OUTPUT() GPIO_PinModeSet(TFT_PORT_D0,TFT_PIN_D0, gpioModePushPull, 1)
#define TFT_PIN_D0_INPUT()  GPIO_PinModeSet(TFT_PORT_D0,TFT_PIN_D0, gpioModeInput, 0)



#endif
/*********************************Hardware dependent part - END*****************************************/

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
