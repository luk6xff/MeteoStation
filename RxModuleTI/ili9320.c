/*
 * .cpp
 *
 *  Created on: 30-08-2016
 *      Author: igbt6
 */

#include "ILI9320.h"
#include <math.h>
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"

#define Delay(ms)  MAP_SysCtlDelay( (MAP_SysCtlClockGet()/(3*1000))*ms);

/*********************************Hardware dependent part*****************************************/
/*********************************Hardware dependent part*****************************************/

static inline void setPin(unsigned long port, unsigned char pin,
		unsigned char val) {
	GPIOPinWrite(port, pin, val & 0xFF ? pin : 0);
}

static inline uint16_t getPin(unsigned long port, unsigned char pin) {
	return GPIOPinRead(port, pin);
}

#define CLOCKS_ENABLE() {	\
							SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF); \
							SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE); \
							SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD); \
							SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB); \
							SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC); \
						}
//==================/CS=====================
#define CS_PIN      GPIO_PIN_6//
#define CS_PORT     GPIO_PORTD_BASE
#define CS_OUTPUT() GPIOPinTypeGPIOOutput(CS_PORT, CS_PIN)
#define CS_HIGH()   setPin(CS_PORT , CS_PIN, 1)
#define CS_LOW()    setPin(CS_PORT , CS_PIN, 0)
//------------------RS----------------------

#define RS_PIN      GPIO_PIN_1
#define RS_PORT     GPIO_PORTE_BASE
#define RS_OUTPUT() GPIOPinTypeGPIOOutput(RS_PORT, RS_PIN)
#define RS_HIGH()   setPin(RS_PORT , RS_PIN, 1)
#define RS_LOW()    setPin(RS_PORT , RS_PIN, 0)

//------------------WR----------------------

#define WR_PIN      GPIO_PIN_2
#define WR_PORT     GPIO_PORTE_BASE
#define WR_OUTPUT() GPIOPinTypeGPIOOutput(WR_PORT, WR_PIN)
#define WR_HIGH()   setPin(WR_PORT, WR_PIN, 1)
#define WR_LOW()    setPin(WR_PORT, WR_PIN, 0)
#define WR_RISING() {WR_HIGH(); WR_LOW(); }
//------------------RD---------------------

#define RD_PIN      GPIO_PIN_3
#define RD_PORT     GPIO_PORTE_BASE
#define RD_OUTPUT() GPIOPinTypeGPIOOutput(RD_PORT, RD_PIN)
#define RD_HIGH()   setPin(RD_PORT, RD_PIN, 1)
#define RD_LOW()    setPin(RD_PORT, RD_PIN, 0)
#define RD_RISING() {RD_HIGH(); RD_LOW(); }

//------------------TRANSISTOR---------------------
#define LCD_TRANS_ENABLE_PORT GPIO_PORTF_BASE
#define LCD_TRANS_ENABLE_PIN GPIO_PIN_4
#define LCD_TRANS_ENABLE_OUTPUT() GPIOPinTypeGPIOOutput(LCD_TRANS_ENABLE_PORT,LCD_TRANS_ENABLE_PIN)
#define LCD_TRANS_ENABLE_INPUT()  GPIOPinTypeGPIOInput(LCD_TRANS_ENABLE_PORT,LCD_TRANS_ENABLE_PIN)
#define LCD_TRANS_HIGH()   	setPin(LCD_TRANS_ENABLE_PORT , LCD_TRANS_ENABLE_PIN, 1)
#define LCD_TRANS_LOW()    	setPin(LCD_TRANS_ENABLE_PORT , LCD_TRANS_ENABLE_PIN, 0)

//------------------DATA-------------------
#define TFT_PORT_D15 GPIO_PORTC_BASE//
#define TFT_PIN_D15 GPIO_PIN_7//
#define TFT_PIN_D15_OUTPUT() GPIOPinTypeGPIOOutput(TFT_PORT_D15,TFT_PIN_D15)
#define TFT_PIN_D15_INPUT()  GPIOPinTypeGPIOInput(TFT_PORT_D15,TFT_PIN_D15)

#define TFT_PORT_D14 GPIO_PORTC_BASE//
#define TFT_PIN_D14 GPIO_PIN_6//
#define TFT_PIN_D14_OUTPUT() GPIOPinTypeGPIOOutput(TFT_PORT_D14,TFT_PIN_D14)
#define TFT_PIN_D14_INPUT()  GPIOPinTypeGPIOInput(TFT_PORT_D14,TFT_PIN_D14)

#define TFT_PORT_D13 GPIO_PORTC_BASE//
#define TFT_PIN_D13 GPIO_PIN_5//
#define TFT_PIN_D13_OUTPUT() GPIOPinTypeGPIOOutput(TFT_PORT_D13,TFT_PIN_D13)
#define TFT_PIN_D13_INPUT()  GPIOPinTypeGPIOInput(TFT_PORT_D13,TFT_PIN_D13)

#define TFT_PORT_D12 GPIO_PORTC_BASE//
#define TFT_PIN_D12 GPIO_PIN_4//
#define TFT_PIN_D12_OUTPUT() GPIOPinTypeGPIOOutput(TFT_PORT_D12,TFT_PIN_D12)
#define TFT_PIN_D12_INPUT()  GPIOPinTypeGPIOInput(TFT_PORT_D12,TFT_PIN_D12)

#define TFT_PORT_D11 GPIO_PORTD_BASE//
#define TFT_PIN_D11 GPIO_PIN_3//
#define TFT_PIN_D11_OUTPUT() GPIOPinTypeGPIOOutput(TFT_PORT_D11,TFT_PIN_D11)
#define TFT_PIN_D11_INPUT()  GPIOPinTypeGPIOInput(TFT_PORT_D11,TFT_PIN_D11)

#define TFT_PORT_D10 GPIO_PORTD_BASE//
#define TFT_PIN_D10 GPIO_PIN_2//
#define TFT_PIN_D10_OUTPUT() GPIOPinTypeGPIOOutput(TFT_PORT_D10,TFT_PIN_D10)
#define TFT_PIN_D10_INPUT()  GPIOPinTypeGPIOInput(TFT_PORT_D10,TFT_PIN_D10)

#define TFT_PORT_D9 GPIO_PORTD_BASE//
#define TFT_PIN_D9  GPIO_PIN_1//
#define TFT_PIN_D9_OUTPUT() GPIOPinTypeGPIOOutput(TFT_PORT_D9,TFT_PIN_D9)
#define TFT_PIN_D9_INPUT()  GPIOPinTypeGPIOInput(TFT_PORT_D9,TFT_PIN_D9)

#define TFT_PORT_D8 GPIO_PORTD_BASE//
#define TFT_PIN_D8  GPIO_PIN_0//
#define TFT_PIN_D8_OUTPUT() GPIOPinTypeGPIOOutput(TFT_PORT_D8,TFT_PIN_D8)
#define TFT_PIN_D8_INPUT()  GPIOPinTypeGPIOInput(TFT_PORT_D8,TFT_PIN_D8)

#define TFT_PORT_D7 GPIO_PORTB_BASE//
#define TFT_PIN_D7 GPIO_PIN_7//
#define TFT_PIN_D7_OUTPUT() GPIOPinTypeGPIOOutput(TFT_PORT_D7,TFT_PIN_D7)
#define TFT_PIN_D7_INPUT()  GPIOPinTypeGPIOInput(TFT_PORT_D7,TFT_PIN_D7)

#define TFT_PORT_D6 GPIO_PORTE_BASE//
#define TFT_PIN_D6 GPIO_PIN_0//
#define TFT_PIN_D6_OUTPUT() GPIOPinTypeGPIOOutput(TFT_PORT_D6,TFT_PIN_D6)
#define TFT_PIN_D6_INPUT()  GPIOPinTypeGPIOInput(TFT_PORT_D6,TFT_PIN_D6)

#define TFT_PORT_D5 GPIO_PORTB_BASE//
#define TFT_PIN_D5 GPIO_PIN_5//
#define TFT_PIN_D5_OUTPUT() GPIOPinTypeGPIOOutput(TFT_PORT_D5,TFT_PIN_D5)
#define TFT_PIN_D5_INPUT()  GPIOPinTypeGPIOInput(TFT_PORT_D5,TFT_PIN_D5)

#define TFT_PORT_D4 GPIO_PORTB_BASE//
#define TFT_PIN_D4 GPIO_PIN_4//
#define TFT_PIN_D4_OUTPUT() GPIOPinTypeGPIOOutput(TFT_PORT_D4,TFT_PIN_D4)
#define TFT_PIN_D4_INPUT()  GPIOPinTypeGPIOInput(TFT_PORT_D4,TFT_PIN_D4)

#define TFT_PORT_D3 GPIO_PORTB_BASE//
#define TFT_PIN_D3 GPIO_PIN_3//
#define TFT_PIN_D3_OUTPUT() GPIOPinTypeGPIOOutput(TFT_PORT_D3,TFT_PIN_D3)
#define TFT_PIN_D3_INPUT()  GPIOPinTypeGPIOInput(TFT_PORT_D3,TFT_PIN_D3)

#define TFT_PORT_D2 GPIO_PORTB_BASE//
#define TFT_PIN_D2 GPIO_PIN_2//
#define TFT_PIN_D2_OUTPUT() GPIOPinTypeGPIOOutput(TFT_PORT_D2,TFT_PIN_D2)
#define TFT_PIN_D2_INPUT()  GPIOPinTypeGPIOInput(TFT_PORT_D2,TFT_PIN_D2)

#define TFT_PORT_D1 GPIO_PORTB_BASE//
#define TFT_PIN_D1 GPIO_PIN_1//
#define TFT_PIN_D1_OUTPUT() GPIOPinTypeGPIOOutput(TFT_PORT_D1,TFT_PIN_D1)
#define TFT_PIN_D1_INPUT()  GPIOPinTypeGPIOInput(TFT_PORT_D1,TFT_PIN_D1)

#define TFT_PORT_D0 GPIO_PORTB_BASE//
#define TFT_PIN_D0 GPIO_PIN_0//
#define TFT_PIN_D0_OUTPUT() GPIOPinTypeGPIOOutput(TFT_PORT_D0,TFT_PIN_D0)
#define TFT_PIN_D0_INPUT()  GPIOPinTypeGPIOInput(TFT_PORT_D0,TFT_PIN_D0)

/*********************************Hardware dependent part - END*****************************************/

/////////////////////////////////////////////////////////////////////////////
//privates
/////////////////////////////////////////////////////////////////////////////
static TransferMode m_transferMode = _16_BIT;
static Orientation m_orientation = LANDSCAPE;
static Resolution m_resolution;
static Font m_currentFont;
static DispalySize m_dispSize;

static void setResolution(uint16_t displayWidth, uint16_t displayHeight) {
	m_resolution.x = displayWidth;  //DISPLAY_WIDTH_X,
	m_resolution.y = displayHeight; //DISPLAY_HEIGHT_Y;
	m_resolution.maxX = m_resolution.x - 1;
	m_resolution.maxY = m_resolution.y - 1;
}

static Resolution getResolution() {
	return m_resolution;
}

static DispalySize getSize() {
	return m_dispSize;
}

static Orientation getScreenOrientation() {
	return m_orientation;
}

static void swapInt(int *a, int*b);

static void allDataPinsInput(void) {
	TFT_PIN_D15_INPUT();
	TFT_PIN_D14_INPUT();
	TFT_PIN_D13_INPUT();
	TFT_PIN_D12_INPUT();
	TFT_PIN_D11_INPUT();
	TFT_PIN_D10_INPUT();
	TFT_PIN_D9_INPUT();
	TFT_PIN_D8_INPUT();
	TFT_PIN_D7_INPUT();
	TFT_PIN_D6_INPUT();
	TFT_PIN_D5_INPUT();
	TFT_PIN_D4_INPUT();
	TFT_PIN_D3_INPUT();
	TFT_PIN_D2_INPUT();
	TFT_PIN_D1_INPUT();
	TFT_PIN_D0_INPUT();
}

static void allDataPinsOutput(void) {
	TFT_PIN_D15_OUTPUT();
	TFT_PIN_D14_OUTPUT();
	TFT_PIN_D13_OUTPUT();
	TFT_PIN_D12_OUTPUT();
	TFT_PIN_D11_OUTPUT();
	TFT_PIN_D10_OUTPUT();
	TFT_PIN_D9_OUTPUT();
	TFT_PIN_D8_OUTPUT();
	TFT_PIN_D7_OUTPUT();
	TFT_PIN_D6_OUTPUT();
	TFT_PIN_D5_OUTPUT();
	TFT_PIN_D4_OUTPUT();
	TFT_PIN_D3_OUTPUT();
	TFT_PIN_D2_OUTPUT();
	TFT_PIN_D1_OUTPUT();
	TFT_PIN_D0_OUTPUT();
}

static void allDataPinsLow(void) {
	setPin(TFT_PORT_D15, TFT_PIN_D15, 0);
	setPin(TFT_PORT_D14, TFT_PIN_D14, 0);
	setPin(TFT_PORT_D13, TFT_PIN_D13, 0);
	setPin(TFT_PORT_D12, TFT_PIN_D12, 0);
	setPin(TFT_PORT_D11, TFT_PIN_D11, 0);
	setPin(TFT_PORT_D10, TFT_PIN_D10, 0);
	setPin(TFT_PORT_D9, TFT_PIN_D9, 0);
	setPin(TFT_PORT_D8, TFT_PIN_D8, 0);
	setPin(TFT_PORT_D7, TFT_PIN_D7, 0);
	setPin(TFT_PORT_D6, TFT_PIN_D6, 0);
	setPin(TFT_PORT_D5, TFT_PIN_D5, 0);
	setPin(TFT_PORT_D4, TFT_PIN_D4, 0);
	setPin(TFT_PORT_D3, TFT_PIN_D3, 0);
	setPin(TFT_PORT_D2, TFT_PIN_D2, 0);
	setPin(TFT_PORT_D1, TFT_PIN_D1, 0);
	setPin(TFT_PORT_D0, TFT_PIN_D0, 0);
}

static void pushData(uint16_t data) {
	setPin(TFT_PORT_D0, TFT_PIN_D0, data & 0x01);
	setPin(TFT_PORT_D1, TFT_PIN_D1, (data >> 1) & 0x01);
	setPin(TFT_PORT_D2, TFT_PIN_D2, (data >> 2) & 0x01);
	setPin(TFT_PORT_D3, TFT_PIN_D3, (data >> 3) & 0x01);
	setPin(TFT_PORT_D4, TFT_PIN_D4, (data >> 4) & 0x01);
	setPin(TFT_PORT_D5, TFT_PIN_D5, (data >> 5) & 0x01);
	setPin(TFT_PORT_D6, TFT_PIN_D6, (data >> 6) & 0x01);
	setPin(TFT_PORT_D7, TFT_PIN_D7, (data >> 7) & 0x01);
	setPin(TFT_PORT_D8, TFT_PIN_D8, (data >> 8) & 0x01);
	setPin(TFT_PORT_D9, TFT_PIN_D9, (data >> 9) & 0x01);
	setPin(TFT_PORT_D10, TFT_PIN_D10, (data >> 10) & 0x01);
	setPin(TFT_PORT_D11, TFT_PIN_D11, (data >> 11) & 0x01);
	setPin(TFT_PORT_D12, TFT_PIN_D12, (data >> 12) & 0x01);
	setPin(TFT_PORT_D13, TFT_PIN_D13, (data >> 13) & 0x01);
	setPin(TFT_PORT_D14, TFT_PIN_D14, (data >> 14) & 0x01);
	setPin(TFT_PORT_D15, TFT_PIN_D15, (data >> 15) & 0x01);
}

static uint16_t getData(void) {
	uint16_t data = 0;
	Delay(100);
	data |= getPin(TFT_PORT_D15, TFT_PIN_D15) << 15;
	data |= getPin(TFT_PORT_D14, TFT_PIN_D14) << 14;
	data |= getPin(TFT_PORT_D13, TFT_PIN_D13) << 13;
	data |= getPin(TFT_PORT_D12, TFT_PIN_D12) << 12;
	data |= getPin(TFT_PORT_D11, TFT_PIN_D11) << 11;
	data |= getPin(TFT_PORT_D10, TFT_PIN_D10) << 10;
	data |= getPin(TFT_PORT_D9, TFT_PIN_D9) << 9;
	data |= getPin(TFT_PORT_D8, TFT_PIN_D8) << 8;
	data |= getPin(TFT_PORT_D7, TFT_PIN_D7) << 7;
	data |= getPin(TFT_PORT_D6, TFT_PIN_D6) << 6;
	data |= getPin(TFT_PORT_D5, TFT_PIN_D5) << 5;
	data |= getPin(TFT_PORT_D4, TFT_PIN_D4) << 4;
	data |= getPin(TFT_PORT_D3, TFT_PIN_D3) << 3;
	data |= getPin(TFT_PORT_D2, TFT_PIN_D2) << 2;
	data |= getPin(TFT_PORT_D1, TFT_PIN_D1) << 1;
	data |= getPin(TFT_PORT_D0, TFT_PIN_D0) << 0;
	return data;
}

static void SendCommand(uint16_t index) {
	RS_LOW();
	RD_HIGH();
	WR_HIGH();
	WR_LOW();
	pushData(index);
	WR_HIGH();
}

static void SendData(uint16_t data) {
	RS_HIGH();
	RD_HIGH();
	WR_LOW();
	pushData(data);
	WR_HIGH();
}

static void WriteData(uint16_t data) {
	CS_LOW();
	SendData(data);
	CS_HIGH();
}

static void WriteCommand(uint16_t data) {
	CS_LOW();
	SendCommand(data);
	CS_HIGH();
}

/************************************************************************
 * void WriteRegister(uint16_t index, uint16_t data)
 **                                                                    **
 ** CS       ----\__________________________________________/-------  **
 ** RS       ------\____________/-----------------------------------  **
 ** RD       -------------------------------------------------------  **
 ** WR       --------\_______/--------\_____/-----------------------  **
 ** DB[15:0] ---------[index]----------[data]-----------------------  **
 **                                                                    **
 ************************************************************************/
static void WriteRegister(uint16_t index, uint16_t data) {
	CS_LOW();
	SendCommand(index);
	SendData(data);
	CS_HIGH();

}
/***********************************************************************
 * uint16_t ReadRegister(uint16_t index)      (16BIT)          **
 **                                                                    **
 ** nCS       ----\__________________________________________/-------  **
 ** RS        ------\____________/-----------------------------------  **
 ** nRD       -------------------------\_____/---------------------  **
 ** nWR       --------\_______/--------------------------------------  **
 ** DB[15:0]  ---------[index]----------[data]-----------------------  **
 **                                                                    **
 ************************************************************************/
uint16_t ReadRegister(uint16_t index)
{
	uint16_t data = 0;

	CS_LOW();
	RS_LOW();

	WR_LOW();
	pushData(index);
	WR_HIGH();

	allDataPinsInput();
	allDataPinsLow();
	RS_HIGH();

	RD_LOW();
	RD_HIGH();
	data = getData();

	CS_HIGH();
	allDataPinsOutput();
	return data;
}

void lcdWriteCOMMAND(uint16_t data) {
	SendCommand(data);
}

void lcdWriteDATA(uint16_t data) {
	SendData(data);
}

/************************************************************************
 * void lcdWriteCOMMAND_DATA(uint16_t command, uint16_t data)
 **                                                                    **
 ** CS       ----\__________________________________________/-------  **
 ** RS       ------\____________/-----------------------------------  **
 ** RD       -------------------------------------------------------  **
 ** WR       --------\_______/--------\_____/-----------------------  **
 ** DB[15:0] ---------[command]----------[data]-----------------------  **
 **                                                                    **
 ************************************************************************/
void lcdWriteCOMMAND_DATA(uint16_t command, uint16_t data)
{
	SendCommand(command);
	SendData(data);
}

void init(TransferMode tMode, Orientation o)
{

	m_orientation = o;
	setResolution(240, 320);
	m_transferMode = tMode;

	CLOCKS_ENABLE()
	;

	LCD_TRANS_ENABLE_OUTPUT(); //LCD ENABLE
	LCD_TRANS_HIGH() ;
	Delay(1);
	CS_OUTPUT();
	Delay(1);
	RD_OUTPUT();
	Delay(1);
	WR_OUTPUT();
	Delay(1);
	RS_OUTPUT();
	Delay(1);
	allDataPinsOutput();
	allDataPinsLow();
	Delay(1);
	//ILI9320 LCD INIT
	WriteRegister(0xe5, 0x8000);
	Delay(500);
	WriteRegister(0x00, 0x0001);
	WriteRegister(0x01, 0x0100);	//Driver Output Contral.
	WriteRegister(0x02, 0x0700);	//LCD Driver Waveform Contral.
	WriteRegister(0x03, 0x1030);	//Entry Mode Set.

	WriteRegister(0x04, 0x0000);	//Scalling Control.
	WriteRegister(0x08, 0x0202);	//Display Control 2.(0x0207)
	WriteRegister(0x09, 0x0000);	//Display Control 3.(0x0000)
	WriteRegister(0x0a, 0x0000);	//Frame Cycle Contal.(0x0000)
	WriteRegister(0x0c, 0x0000); //Extern Display Interface Control 1.(0x0000)
	WriteRegister(0x0d, 0x0000);	//Frame Maker Position.
	WriteRegister(0x0f, 0x0000);   //Extern Display Interface Control 2.
	Delay(200);
	//********Power On sequence*******************
	WriteRegister(0x10, 0x0000);   //Power Control 1
	WriteRegister(0x11, 0x0007);   //Power Control 2
	WriteRegister(0x12, 0x0000);   //Power Control 3
	WriteRegister(0x13, 0x0000);   //Power Control 4
	Delay(50);
	WriteRegister(0x10, 0x17B0);
	WriteRegister(0x11, 0x0007);
	Delay(10);
	WriteRegister(0x12, 0x013A);
	Delay(10);
	WriteRegister(0x13, 0x1A00);
	WriteRegister(0x29, 0x000c);   //Power Control 7
	Delay(10);

	//********Gamma control***********************
	WriteRegister(0x30, 0x0000);
	WriteRegister(0x31, 0x0505);
	WriteRegister(0x32, 0x0004);
	WriteRegister(0x35, 0x0006);
	WriteRegister(0x36, 0x0707);
	WriteRegister(0x37, 0x0105);
	WriteRegister(0x38, 0x0002);
	WriteRegister(0x39, 0x0707);
	WriteRegister(0x3C, 0x0704);
	WriteRegister(0x3D, 0x0807);

	//********Set RAM area************************
	WriteRegister(0x50, 0x0000);   //Set X Start.
	WriteRegister(0x51, m_resolution.maxX);   //Set X End. (239)
	WriteRegister(0x52, 0x0000);   //Set Y Start.
	WriteRegister(0x53, m_resolution.maxY);   //Set Y End. (319)
	WriteRegister(0x60, 0x2700);   //Driver Output Control.
	WriteRegister(0x61, 0x0001);   //Driver Output Control.
	WriteRegister(0x6A, 0x0000);   //Vertical Srcoll Control.
	WriteRegister(0x21, 0x0000);
	WriteRegister(0x20, 0x0000);
	//********Partial Display Control*********
	WriteRegister(0x80, 0x0000);  //Display Position? Partial Display 1.
	WriteRegister(0x81, 0x0000); //RAM Address Start? Partial Display 1.
	WriteRegister(0x82, 0x0000);	//RAM Address End-Partial Display 1.
	WriteRegister(0x83, 0x0000);	//Displsy Position? Partial Display 2.
	WriteRegister(0x84, 0x0000);	//RAM Address Start? Partial Display 2.
	WriteRegister(0x85, 0x0000);	//RAM Address End? Partial Display 2.
	//********Panel Control******************
	WriteRegister(0x90, 0x0010);	//Frame Cycle Contral.(0x0013)
	WriteRegister(0x92, 0x0000);	//Panel Interface Contral 2.(0x0000)
	WriteRegister(0x93, 0x0003);	//Panel Interface Contral 3.
	WriteRegister(0x95, 0x0110);	//Frame Cycle Contral.(0x0110)
	WriteRegister(0x97, 0x0000);	//
	WriteRegister(0x98, 0x0000);	//Frame Cycle Contral.
	//********Display On******************
	Delay(100);
	WriteRegister(0x07, 0x0173);
	Delay(100);
	WriteCommand(0x22);
	fillScreenBackground(RED);
	//setFont(BigFont);

}

void setXY(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
	if (m_orientation == LANDSCAPE) {
		uint16_t temp = y1;
		y1 = x1;
		x1 = temp;
		temp = y2;
		y2 = x2;
		x2 = temp;
		y1 = m_resolution.maxY - y1;
		y2 = m_resolution.maxY - y2;

		temp = y2;
		y2 = y1;
		y1 = temp;
	}
	lcdWriteCOMMAND_DATA(0x50, x1);
	lcdWriteCOMMAND_DATA(0x52, y1);
	lcdWriteCOMMAND_DATA(0x51, x2);
	lcdWriteCOMMAND_DATA(0x53, y2);
	lcdWriteCOMMAND_DATA(0x20, x1);
	lcdWriteCOMMAND_DATA(0x21, y1);
	lcdWriteCOMMAND(0x22);
}

void clrXY() {
	CS_LOW();
	if (m_orientation == PORTRAIT)
		setXY(0, 0, m_resolution.maxX, m_resolution.maxY);
	else
		setXY(0, 0, m_resolution.maxY, m_resolution.maxX);
	CS_HIGH();
}

void drawHLine(int x, int y, int l, uint16_t color) {
	if (l < 0) {
		l = -l;
		x -= l;
	}
	CS_LOW();
	setXY(x, y, x + l, y);
	for (int i = 0; i < l + 1; i++) {
		lcdWriteDATA(color);
	}
	CS_HIGH();
	clrXY();
}

void drawVLine(int x, int y, int l, uint16_t color)
{
	if (l < 0)
	{
		l = -l;
		y -= l;
	}
	CS_LOW();
	setXY(x, y, x, y + l);

	for (int i = 0; i < l + 1; i++)
	{
		lcdWriteDATA(color);
	}
	CS_HIGH();
	clrXY();
}

void drawLine(int x1, int y1, int x2, int y2, uint16_t color)
{
	if (y1 == y2)
		drawHLine(x1, y1, x2 - x1, color);
	else if (x1 == x2)
		drawVLine(x1, y1, y2 - y1, color);
	else {
		unsigned int dx = (x2 > x1 ? x2 - x1 : x1 - x2);
		short xstep = x2 > x1 ? 1 : -1;
		unsigned int dy = (y2 > y1 ? y2 - y1 : y1 - y2);
		short ystep = y2 > y1 ? 1 : -1;
		int col = x1, row = y1;

		CS_LOW();
		if (dx < dy) {
			int t = -(dy >> 1);
			while (true) {
				setXY(col, row, col, row);
				lcdWriteDATA(color);
				if (row == y2)
					return;
				row += ystep;
				t += dx;
				if (t >= 0) {
					col += xstep;
					t -= dy;
				}
			}
		} else {
			int t = -(dx >> 1);
			while (true) {
				setXY(col, row, col, row);
				lcdWriteDATA(color);
				if (col == x2)
					return;
				col += xstep;
				t += dy;
				if (t >= 0) {
					row += ystep;
					t -= dx;
				}
			}
		}
		CS_HIGH();
	}
	clrXY();
}

void setColor(uint8_t r, uint8_t g, uint8_t b)
{
	lcdWriteDATA(r | g << 1 | b << 2);	// rrrrrggggggbbbbb
}

void setPixel(uint16_t color)
{
	lcdWriteDATA(color);	// rrrrrggggggbbbbb
}

void drawPixel(int x, int y, uint16_t color) {
	CS_LOW();
	setXY(x, y, x, y);
	setPixel(color);
	CS_HIGH();
	clrXY();
}

void drawRect(int x1, int y1, int x2, int y2, uint16_t color)
{
	if (x1 > x2) {
		int temp = x1;
		x1 = x2;
		x2 = temp;
	}
	if (y1 > y2) {
		int temp = y1;
		y1 = y2;
		y2 = temp;
	}

	drawHLine(x1, y1, x2 - x1, color);
	drawHLine(x1, y2, x2 - x1, color);
	drawVLine(x1, y1, y2 - y1, color);
	drawVLine(x2, y1, y2 - y1, color);
}

void drawRoundRect(int x1, int y1, int x2, int y2, uint16_t color)
{
	if (x1 > x2) {
		int temp = x1;
		x1 = x2;
		x2 = temp;
	}
	if (y1 > y2) {
		int temp = y1;
		y1 = y2;
		y2 = temp;
	}
	if ((x2 - x1) > 4 && (y2 - y1) > 4) {
		drawPixel(x1 + 1, y1 + 1, color);
		drawPixel(x2 - 1, y1 + 1, color);
		drawPixel(x1 + 1, y2 - 1, color);
		drawPixel(x2 - 1, y2 - 1, color);
		drawHLine(x1 + 2, y1, x2 - x1 - 4, color);
		drawHLine(x1 + 2, y2, x2 - x1 - 4, color);
		drawVLine(x1, y1 + 2, y2 - y1 - 4, color);
		drawVLine(x2, y1 + 2, y2 - y1 - 4, color);
	}
}

void fillRect(int x1, int y1, int x2, int y2, uint16_t color) {
	if (x1 > x2) {
		int temp = x1;
		x1 = x2;
		x2 = temp;
	}
	if (y1 > y2) {
		int temp = y1;
		y1 = y2;
		y2 = temp;
	}
	if (m_orientation == PORTRAIT) {
		for (int i = 0; i < ((y2 - y1) / 2) + 1; i++) {
			drawHLine(x1, y1 + i, x2 - x1, color);
			drawHLine(x1, y2 - i, x2 - x1, color);
		}
	} else {
		for (int i = 0; i < ((x2 - x1) / 2) + 1; i++) {
			drawVLine(x1 + i, y1, y2 - y1, color);
			drawVLine(x2 - i, y1, y2 - y1, color);
		}
	}
}

void fillRoundRect(int x1, int y1, int x2, int y2, uint16_t color) {
	if (x1 > x2) {
		int temp = x1;
		x1 = x2;
		x2 = temp;
	}
	if (y1 > y2) {
		int temp = y1;
		y1 = y2;
		y2 = temp;
	}

	if ((x2 - x1) > 4 && (y2 - y1) > 4) {
		for (int i = 0; i < ((y2 - y1) / 2) + 1; i++) {
			switch (i) {
			case 0:
				drawHLine(x1 + 2, y1 + i, x2 - x1 - 4, color);
				drawHLine(x1 + 2, y2 - i, x2 - x1 - 4, color);
				break;
			case 1:
				drawHLine(x1 + 1, y1 + i, x2 - x1 - 2, color);
				drawHLine(x1 + 1, y2 - i, x2 - x1 - 2, color);
				break;
			default:
				drawHLine(x1, y1 + i, x2 - x1, color);
				drawHLine(x1, y2 - i, x2 - x1, color);
			}
		}
	}
}

void drawCircle(int x, int y, int radius, uint16_t color) {
	int f = 1 - radius;
	int ddF_x = 1;
	int ddF_y = -2 * radius;
	int x1 = 0;
	int y1 = radius;

	CS_LOW();
	setXY(x, y + radius, x, y + radius);
	lcdWriteDATA(color);
	setXY(x, y - radius, x, y - radius);
	lcdWriteDATA(color);
	setXY(x + radius, y, x + radius, y);
	lcdWriteDATA(color);
	setXY(x - radius, y, x - radius, y);
	lcdWriteDATA(color);

	while (x1 < y1) {
		if (f >= 0) {
			y1--;
			ddF_y += 2;
			f += ddF_y;
		}
		x1++;
		ddF_x += 2;
		f += ddF_x;
		setXY(x + x1, y + y1, x + x1, y + y1);
		lcdWriteDATA(color);
		setXY(x - x1, y + y1, x - x1, y + y1);
		lcdWriteDATA(color);
		setXY(x + x1, y - y1, x + x1, y - y1);
		lcdWriteDATA(color);
		setXY(x - x1, y - y1, x - x1, y - y1);
		lcdWriteDATA(color);
		setXY(x + y1, y + x1, x + y1, y + x1);
		lcdWriteDATA(color);
		setXY(x - y1, y + x1, x - y1, y + x1);
		lcdWriteDATA(color);
		setXY(x + y1, y - x1, x + y1, y - x1);
		lcdWriteDATA(color);
		setXY(x - y1, y - x1, x - y1, y - x1);
		lcdWriteDATA(color);
	}
	CS_HIGH();
	clrXY();
}

void fillCircle(int x, int y, int radius, uint16_t color) {
	for (int y1 = -radius; y1 <= 0; y1++)
		for (int x1 = -radius; x1 <= 0; x1++)
			if (x1 * x1 + y1 * y1 <= radius * radius) {
				drawHLine(x + x1, y + y1, 2 * (-x1), color);
				drawHLine(x + x1, y - y1, 2 * (-x1), color);
				break;
			}
}

void clrScr() {
	long i;
	CS_LOW();
	clrXY();
	for (i = 0; i < ((m_resolution.x) * (m_resolution.y)); i++) {
		lcdWriteDATA(0);
	}
	CS_HIGH();
}

void printChar(uint8_t c, int x, int y, uint16_t color) {
	uint8_t i, ch;
	uint8_t j;
	uint16_t temp;
	CS_LOW();
	temp = ((c - m_currentFont.offset)
			* ((m_currentFont.xSize / 8) * m_currentFont.ySize)) + 4;
	for (j = 0; j < m_currentFont.ySize; j++) {
		for (int zz = 0; zz < (m_currentFont.xSize / 8); zz++) {
			ch = m_currentFont.font[temp + zz];
			for (i = 0; i < 8; i++) {
				setXY(x + i + (zz * 8), y + j, x + i + (zz * 8) + 1, y + j + 1);

				if ((ch & (1 << (7 - i))) != 0) {
					setPixel(color);
				}
			}
		}
		temp += (m_currentFont.xSize / 8);
	}
	CS_HIGH();
	clrXY();
}

void rotateChar(uint8_t c, int x, int y, int pos, int deg, uint16_t color) {
	uint8_t i, j, ch;
	uint16_t temp;
	int newx, newy;
	double radian;
	radian = deg * 0.0175;
	CS_LOW();
	temp = ((c - m_currentFont.offset)
			* ((m_currentFont.xSize / 8) * m_currentFont.ySize)) + 4;
	for (j = 0; j < m_currentFont.ySize; j++) {
		for (int zz = 0; zz < (m_currentFont.xSize / 8); zz++) {
			ch = m_currentFont.font[temp + zz];
			for (i = 0; i < 8; i++) {
				newx = x
						+ (((i + (zz * 8) + (pos * m_currentFont.xSize))
								* cos(radian)) - ((j) * sin(radian)));
				newy = y
						+ (((j) * cos(radian))
								+ ((i + (zz * 8) + (pos * m_currentFont.xSize))
										* sin(radian)));

				setXY(newx, newy, newx + 1, newy + 1);

				if ((ch & (1 << (7 - i))) != 0) {
					setPixel(color);
				} else {
					setPixel(color);
				}
			}
		}
		temp += (m_currentFont.xSize / 8);
	}
	CS_HIGH();
	clrXY();
}

void print(char *st, int x, int y, int deg, uint16_t color) {
	int stl, i;

	stl = strlen(st);

	if (m_orientation == PORTRAIT) {
		if (x == RIGHT)
			x = (m_resolution.x) - (stl * m_currentFont.xSize);
		if (x == CENTER)
			x = ((m_resolution.x) - (stl * m_currentFont.xSize)) / 2;
	} else {
		if (x == RIGHT)
			x = (m_resolution.y) - (stl * m_currentFont.xSize);
		if (x == CENTER)
			x = ((m_resolution.y) - (stl * m_currentFont.xSize)) / 2;
	}
	for (i = 0; i < stl; i++) {
		if (deg == 0)
			printChar(*st++, x + (i * (m_currentFont.xSize)), y, color);
		else
			rotateChar(*st++, x, y, i, deg, color);
	}
}

void printNumI(long num, int x, int y, int length, char filler, uint16_t color) {
	char buf[25];
	char st[27];
	bool neg = false;
	int c = 0, f = 0;

	if (num == 0) {
		if (length != 0) {
			for (c = 0; c < (length - 1); c++)
				st[c] = filler;
			st[c] = 48;
			st[c + 1] = 0;
		} else {
			st[0] = 48;
			st[1] = 0;
		}
	} else {
		if (num < 0) {
			neg = true;
			num = -num;
		}

		while (num > 0) {
			buf[c] = 48 + (num % 10);
			c++;
			num = (num - (num % 10)) / 10;
		}
		buf[c] = 0;

		if (neg) {
			st[0] = 45;
		}

		if (length > (c + neg)) {
			for (int i = 0; i < (length - c - neg); i++) {
				st[i + neg] = filler;
				f++;
			}
		}
		for (int i = 0; i < c; i++) {
			st[i + neg + f] = buf[c - i - 1];
		}
		st[c + neg + f] = 0;

	}

	print(st, x, y, 0, color);
}

void convertFloat(char *buf, double num, int width, uint8_t prec) {
	char format[10];

	sprintf(format, "%%%i.%if", width, prec);
	sprintf(buf, format, num);
}

void printNumF(double num, uint8_t dec, int x, int y, char divider, int length,
		char filler, uint16_t color) {
	char st[27];
	bool neg = false;
	if (dec < 1)
		dec = 1;
	else if (dec > 5)
		dec = 5;

	if (num < 0)
		neg = true;
	convertFloat(st, num, length, dec);
	if (divider != '.') {
		for (int i = 0; i < sizeof(st); i++)
			if (st[i] == '.')
				st[i] = divider;
	}

	if (filler != ' ') {
		if (neg) {
			st[0] = '-';
			for (int i = 1; i < sizeof(st); i++)
				if ((st[i] == ' ') || (st[i] == '-'))
					st[i] = filler;
		} else {
			for (int i = 0; i < sizeof(st); i++)
				if (st[i] == ' ')
					st[i] = filler;
		}
	}

	print(st, x, y, 0, color);
}

void drawBitmap(int x, int y, int sx, int sy, const uint16_t* data, int scale) {
	uint16_t col;
	int tx, ty, tc, tsx, tsy;

	if (scale == 1) {
		if (m_orientation == PORTRAIT) {
			CS_LOW();
			setXY(x, y, x + sx - 1, y + sy - 1);
			for (tc = 0; tc < (sx * sy); tc++) {
				col = data[tc];
				lcdWriteDATA(col);
			}
			CS_HIGH();
		} else {
			CS_LOW();
			for (ty = 0; ty < sy; ty++) {
				setXY(x, y + ty, x + sx - 1, y + ty);
				for (tx = sx - 1; tx >= 0; tx--) {
					col = data[(ty * sx) + tx];
					lcdWriteDATA(col);
				}
			}
			CS_HIGH();
		}
	} else {
		if (m_orientation == PORTRAIT) {
			CS_LOW();
			for (ty = 0; ty < sy; ty++) {
				setXY(x, y + (ty * scale), x + ((sx * scale) - 1),
						y + (ty * scale) + scale);
				for (tsy = 0; tsy < scale; tsy++)
					for (tx = 0; tx < sx; tx++) {
						col = data[(ty * sx) + tx];
						for (tsx = 0; tsx < scale; tsx++)
							lcdWriteDATA(col);
					}
			}
			CS_HIGH();
		} else {
			CS_LOW();
			for (ty = 0; ty < sy; ty++) {
				for (tsy = 0; tsy < scale; tsy++) {
					setXY(x, y + (ty * scale) + tsy, x + ((sx * scale) - 1),
							y + (ty * scale) + tsy);
					for (tx = sx - 1; tx >= 0; tx--) {
						col = data[(ty * sx) + tx];
						for (tsx = 0; tsx < scale; tsx++)
							lcdWriteDATA(col);
					}
				}
			}
			CS_HIGH();
		}
	}
	clrXY();
}

void fillScreenBackground(uint16_t color) {
	uint16_t i, f;
	CS_LOW();
	for (i = 0; i < 320; i++) {
		for (f = 0; f < 240; f++) {
			lcdWriteDATA(color);
		}
	}
	CS_HIGH();
}

/*
 void setFont(const Font* font) {
 mCurrentFont.font = font->font;
 mCurrentFont.xSize = font->xSize;
 mCurrentFont.ySize = font->ySize;
 mCurrentFont.offset = font->offset;
 mCurrentFont.numchars = font->numchars;
 }
 */

void setFont(const uint8_t* font) {
	m_currentFont.font = (uint8_t*) font;
	m_currentFont.xSize = font[0];
	m_currentFont.ySize = font[1];
	m_currentFont.offset = font[2];
	m_currentFont.numchars = font[3];
}

uint8_t* getFont() {
	return m_currentFont.font;
}

uint8_t getFontXsize() {
	return m_currentFont.xSize;
}

uint8_t getFontYsize() {
	return m_currentFont.ySize;
}

static void swapInt(int *a, int*b) {
	int temp = *a;
	*b = temp;
	*a = *b;
}

void ILI9320PixelDrawMultiple(void *pvDisplayData, long lX,
                                           long lY, long lX0, long lCount,
                                           long lBPP,
                                           const unsigned char *pucData,
                                           const unsigned char *pucPalette)
{
    unsigned long ulByte;

    //
    // Set the cursor increment to left to right, followed by top to bottom.
    //
    WriteCommand(0x03);
#ifdef PORTRAIT
    WriteData(0x0030);
#else
//LANDSCAPE
    WriteData(0x0028);
#endif

    //
    // Set the starting X address of the display cursor.
    //
    WriteCommand(0x20);
#ifdef PORTRAIT
    WriteData(lX);
#else
//LANDSCAPE
    WriteData(239 - lY);
#endif

    //
    // Set the Y address of the display cursor.
    //
    WriteCommand(0x21);
#ifdef PORTRAIT
    WriteData(lY);
#else
//LANDSCAPE
    WriteData(lX);
#endif


    //
    // Write the data RAM write command.
    //
    WriteCommand(0x22);

    //
    // Determine how to interpret the pixel data based on the number of bits
    // per pixel.
    //
    switch(lBPP)
    {
        //
        // The pixel data is in 1 bit per pixel format.
        //
        case 1:
        {
            //
            // Loop while there are more pixels to draw.
            //
            while(lCount)
            {
                //
                // Get the next byte of image data.
                //
                ulByte = *pucData++;

                //
                // Loop through the pixels in this byte of image data.
                //
                for(; (lX0 < 8) && lCount; lX0++, lCount--)
                {
                    //
                    // Draw this pixel in the appropriate color.
                    //
                    WriteData(((unsigned long *)pucPalette)[(ulByte >>
                                                             (7 - lX0)) & 1]);
                }

                //
                // Start at the beginning of the next byte of image data.
                //
                lX0 = 0;
            }

            //
            // The image data has been drawn.
            //
            break;
        }

        //
        // The pixel data is in 4 bit per pixel format.
        //
        case 4:
        {
            //
            // Loop while there are more pixels to draw.  "Duff's device" is
            // used to jump into the middle of the loop if the first nibble of
            // the pixel data should not be used.  Duff's device makes use of
            // the fact that a case statement is legal anywhere within a
            // sub-block of a switch statement.  See
            // http://en.wikipedia.org/wiki/Duff's_device for detailed
            // information about Duff's device.
            //
            switch(lX0 & 1)
            {
                case 0:
                    while(lCount)
                    {
                        //
                        // Get the upper nibble of the next byte of pixel data
                        // and extract the corresponding entry from the
                        // palette.
                        //
                        ulByte = (*pucData >> 4) * 3;
                        ulByte = (*(unsigned long *)(pucPalette + ulByte) &
                                  0x00ffffff);

                        //
                        // Translate this palette entry and write it to the
                        // screen.
                        //
                        WriteData(DPYCOLORTRANSLATE(ulByte));

                        //
                        // Decrement the count of pixels to draw.
                        //
                        lCount--;

                        //
                        // See if there is another pixel to draw.
                        //
                        if(lCount)
                        {
                case 1:
                            //
                            // Get the lower nibble of the next byte of pixel
                            // data and extract the corresponding entry from
                            // the palette.
                            //
                            ulByte = (*pucData++ & 15) * 3;
                            ulByte = (*(unsigned long *)(pucPalette + ulByte) &
                                      0x00ffffff);

                            //
                            // Translate this palette entry and write it to the
                            // screen.
                            //
                            WriteData(DPYCOLORTRANSLATE(ulByte));

                            //
                            // Decrement the count of pixels to draw.
                            //
                            lCount--;
                        }
                    }
            }

            //
            // The image data has been drawn.
            //
            break;
        }

        //
        // The pixel data is in 8 bit per pixel format.
        //
        case 8:
        {
            //
            // Loop while there are more pixels to draw.
            //
            while(lCount--)
            {
                //
                // Get the next byte of pixel data and extract the
                // corresponding entry from the palette.
                //
                ulByte = *pucData++ * 3;
                ulByte = *(unsigned long *)(pucPalette + ulByte) & 0x00ffffff;

                //
                // Translate this palette entry and write it to the screen.
                //
                WriteData(DPYCOLORTRANSLATE(ulByte));
            }

            //
            // The image data has been drawn.
            //
            break;
        }
    }
}

