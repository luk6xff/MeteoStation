/**************************************************************************//**
 * @file
 * @brief Meteo Station
 * @author igbt6
 * @version 0.01
 *
 ******************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "ILI9320.h"
#include "ads7843.h"
#include "bitmaps.h"
#include "../emdrv/ustimer/ustimer.h"
#include "ugui.h"
#include "SPFD5408.h"


/*
#include "../3rdParty/AWind/TextBoxString.h"
#include "../3rdParty/AWind/TextBoxNumber.h"
#include "../3rdParty/AWind/DecoratorPrimitives.h"
#include "../3rdParty/AWind/WindowsManager.h"
#include "../3rdParty/AWind/DefaultDecorators.h"
#include "../3rdParty/AWind/defaultFonts.h"

class TextExampleWindow : public MainWindow
{
	TextBoxNumber *_textNumber;
public:
	TextExampleWindow(int width,int height):MainWindow(width,height)
	{
		AddDecorator(new DecoratorRectFill(Color::Black));
		AddDecorator(new DecoratorColor(Color::SkyBlue));
		int x=0;
		int y=40;
		TextBoxFString *label=new TextBoxFString(x,y,width/2,25,("This is label: "));
		label->SetFont(BigFont);
		x=width*3.0/4;
		_textNumber=new TextBoxNumber(x,y,width-x,25,0);
		_textNumber->SetDecorators(GetDecorators()); // here we save one decorator beacuse main window and text window have thae same decorator properties: black background
		_textNumber->SetFont(BigFont);
		_textNumber->SetMargins(20,2);
		_textNumber->SetNumber(4);
		_textNumber->SetIsReadOnly(false);

		AddChild(label);
		AddChild(_textNumber);
	}
	void Create()
	{
	}
	void Notify(Window * wnd)
	{
		if(wnd == _textNumber)
		{
			//out<<F("Value changed")<<((TextBoxNumber *)_textNumber)->GetNumber()<<endln;
		}
	}
};

#include "../3rdParty/AWind/TextBoxString.h"
#include "../3rdParty/AWind/TextBoxStrTouch.h"

class wnd_info : public MainWindow
{
  TextBoxStrTouch *edtBox;
public:
  wnd_info(int width,int height):MainWindow(width,height)
  {
    int x=0, y=10, xOff=95, yOff=20, xW=width-xOff, tHgt=13;
    AddDecorator(new DecoratorRectFill(Color::Black));
    AddDecorator(new DecoratorColor(Color::SkyBlue));

    TextBoxFString *label=new TextBoxFString(x,y,104,tHgt,("Enter text:"));
    AddChild(label);
    edtBox=new TextBoxStrTouch(xOff,y,xW,tHgt,"Some text to edit");
    initEditBox(edtBox);

    y += yOff;
    label=new TextBoxFString(x,y,104,tHgt,("Short Text:"));
    AddChild(label);
    edtBox=new TextBoxStrTouch(xOff,y,xW,tHgt,"Short text");
    initEditBox(edtBox);

    y += yOff;
    label=new TextBoxFString(x,y,104,tHgt,("Long Text:"));
    AddChild(label);
    edtBox=new TextBoxStrTouch(xOff,y,xW,tHgt,"Some long text that is greater than the edit box size will be around");
    initEditBox(edtBox);

    y += yOff;
    label=new TextBoxFString(x,y,104,tHgt,("Long Text:"));
    AddChild(label);
    edtBox=new TextBoxStrTouch(xOff,y,xW,tHgt,"This will exceed the length of the edit area");
    initEditBox(edtBox);
  }
protected:
  void initEditBox(TextBoxStrTouch *edt)
  {
    edt->SetDecorators(GetDecorators());
    edt->SetMargins(4,0);
    AddChild(edt);
  }

public:
	void Create()
	{
	}
	void Notify(Window * wnd)
	{
	}
};
*/

static UG_GUI gui;

#define RGB2RGB565(c) (((((c & 0x00ff0000) >> 16) >> 3) << 11) | \
    ((((c & 0x0000ff00) >> 8) >> 2) << 5) | (((c & 0x000000ff) >> 0) >> 3 ))

static void pixelSet(UG_S16 x, UG_S16 y, UG_COLOR c)
{
    SPFD5408drawPixel(x, y, RGB2RGB565(c));
}

#if 0

static UG_RESULT user_fill_frame(UG_S16 x1, UG_S16 y1, UG_S16 x2, UG_S16 y2,
    UG_COLOR c)
{
    ili9320_fill_frame(x1, y1, x2, y2, RGB2RGB565(c));

    return UG_RESULT_OK;
}


void gui_update()
{
    u16 x, y;

    /* Handle touch screen */
    ads7843_get_xy(&x, &y);
    if (x != -1 && y != -1)
        UG_TouchUpdate(x, y, TOUCH_STATE_PRESSED);
    else
        UG_TouchUpdate(-1, -1, TOUCH_STATE_RELEASED);

    UG_Update();
}
#endif


void WindowCallback(UG_MESSAGE *mess)
{

}
int gui_init()
{
	SPFD5408init();
    UG_Init(&gui, pixelSet, 320, 240);
    UG_SelectGUI(&gui);
	UG_FontSelect( &FONT_12X20 );
	UG_SetForecolor(C_OLIVE);
	UG_SetBackcolor(C_WHITE);
	UG_PutString( 20, 50, (char*)"Hello World");
//    UG_DriverRegister(DRIVER_FILL_FRAME, user_fill_frame);
//    UG_DriverEnable(DRIVER_FILL_FRAME);
	// Use uGui to draw an interface

	#define MAX_OBJS 64
	UG_WINDOW wnd;
	UG_OBJECT objs[MAX_OBJS];
	UG_WindowCreate( &wnd, objs, MAX_OBJS, WindowCallback );
	UG_WindowResize( &wnd, 200, 100, 800, 500);
	UG_WindowShow(&wnd);


	UG_BUTTON btn;
	UG_ButtonCreate(&wnd, &btn, BTN_ID_0, 20, 50, 140, 90 );
	UG_ButtonSetFont(&wnd, BTN_ID_0, &FONT_10X16);
	UG_ButtonSetText(&wnd, BTN_ID_0, (char*)"Push Me");
	UG_ButtonShow(&wnd, BTN_ID_0);

	UG_CHECKBOX cbx;
	UG_CheckboxCreate(&wnd, &cbx, CHB_ID_0, 20, 100, 140, 150 );
	UG_CheckboxSetFont(&wnd, CHB_ID_0, &FONT_10X16);
	UG_CheckboxSetText(&wnd, CHB_ID_0, (char*)"Check Me");
	UG_CheckboxSetCheched(&wnd, CHB_ID_0, CHB_STATE_PRESSED);
	UG_CheckboxShow(&wnd, CHB_ID_0);


	UG_Update();

    return 0;
}

//Sensor Demo
/**************************************************************************//**
 * @brief  Main function
 *****************************************************************************/
int main(void) {
	/* Chip errata */
	CHIP_Init();
	CMU_ClockDivSet(cmuClock_HF, cmuClkDiv_1);
	CMU_OscillatorEnable(cmuOsc_HFXO, true, true);
	CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFXO); //32MHZ
	CMU_ClockEnable(cmuClock_GPIO, true);
	//CMU_ClockEnable(cmuClock_HFPER, true);
	//CMU_ClockDivSet(cmuClock_HFPER, cmuClkDiv_1);
	//CMU_ClockEnable(cmuClock_HFPER, true);
	//----------------------- ILI9320 first working tests -----------------------
	//BSP_TraceProfilerSetup();
	/*BSP_LedsInit();
	 BSP_LedSet(0);
	 BSP_LedSet(1);*/
	// Initialization of USTIMER driver

	USTIMER_Init();
#if 0
	ILI9320 ili9320;
	ili9320.init();
	ADS7843 ads7843;
	ads7843.performThreePointCalibration(ili9320);
#endif

	gui_init();
#if 0
	//Awind Tests
	DefaultDecorators::InitAll();
	//initialize window manager
	//WindowsManager<TextExampleWindow> windowsManager(&ili9320,&ads7843);
	WindowsManager<wnd_info> windowsManager(&ili9320,&ads7843);
	windowsManager.Initialize();
	while(1)
	{
		windowsManager.loop();
	}
#endif
	while (1) {
#if 0
		if (ads7843.dataAvailable())
		{
			ads7843.read();
			TouchPoint p = ads7843.getTouchedPoint();
			ili9320.fillCircle(p.x, p.y,3, ILI9320::Colors::BLUE);
		}
#endif
	}
}

