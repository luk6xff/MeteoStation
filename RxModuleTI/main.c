#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"

#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/systick.h"
#include "driverlib/interrupt.h"
#include "driverlib/fpu.h"

#include "utils/ustdlib.h"
#include "utils/sine.h"

#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "grlib/pushbutton.h"
#include "grlib/checkbox.h"
#include "grlib/container.h"
#include "grlib/radiobutton.h"
#include "grlib/slider.h"

#include "ILI9320_driver.h"
#include "3PointCalibration.h"
#include "config.h"
#include "debugConsole.h"
#include "esp8266.h"
#include "touch.h"

#define RED_LED   GPIO_PIN_1
#define BLUE_LED  GPIO_PIN_2
#define GREEN_LED GPIO_PIN_3

//*****************************************************************************
//
// Forward reference to various widget structures.
//
//*****************************************************************************
extern tCanvasWidget g_sBackground;
extern tPushButtonWidget g_sPushBtn;

//*****************************************************************************
//
// Forward reference to the button press handler.
//
//*****************************************************************************
void OnButtonPress(tWidget *psWidget);

//*****************************************************************************
//
// The canvas widget acting as the background to the display.
//
//*****************************************************************************
Canvas(g_sBackground, WIDGET_ROOT, 0, &g_sPushBtn,
	   &g_ILI9320, 10, 25, 300, (240 - 25 -10),
       CANVAS_STYLE_FILL, ClrAzure, 0, 0, 0, 0, 0, 0);

RectangularButton(g_sPushBtn, &g_sBackground, 0, 0,
		 	 	  &g_ILI9320, 60, 60, 200, 40,
                  (PB_STYLE_OUTLINE | PB_STYLE_TEXT_OPAQUE | PB_STYLE_TEXT |
                  PB_STYLE_FILL | PB_STYLE_RELEASE_NOTIFY),
                  ClrDarkBlue, ClrBlue, ClrWhite, ClrWhite,
                  g_psFontCm20b, "Show Welcome", 0, 0, 0, 0, OnButtonPress);

Canvas(g_sHello, &g_sPushBtn, 0, 0,
	   &g_ILI9320, 10, 150, 300, 40,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT),
       ClrBlack, 0, ClrWhite, g_psFontCm20, "Hello World!", 0, 0);

bool g_bHelloVisible = false;

void OnButtonPress(tWidget *psWidget)
{
    g_bHelloVisible = !g_bHelloVisible;

    if(g_bHelloVisible)
    {
        //
        // Add the Hello widget to the tree as a child of the push
        // button.  We could add it elsewhere but this seems as good a
        // place as any.  It also means we can repaint from g_sPushBtn and
        // this will paint both the button and the welcome message.
        //
        WidgetAdd((tWidget *)&g_sPushBtn, (tWidget *)&g_sHello);

        //
        // Change the button text to indicate the new function.
        //
        PushButtonTextSet(&g_sPushBtn, "Hide Welcome");

        //
        // Repaint the pushbutton and all widgets beneath it (in this case,
        // the welcome message).
        //
        WidgetPaint((tWidget *)&g_sPushBtn);
    }
    else
    {
        //
        // Remove the Hello widget from the tree.
        //
        WidgetRemove((tWidget *)&g_sHello);

        //
        // Change the button text to indicate the new function.
        //
        PushButtonTextSet(&g_sPushBtn, "Show Welcome");

        //
        // Repaint the widget tree to remove the Hello widget from the
        // display.  This is rather inefficient but saves having to use
        // additional widgets to overpaint the area of the Hello text (since
        // disabling a widget does not automatically erase whatever it
        // previously displayed on the screen).
        //
        WidgetPaint(WIDGET_ROOT);
    }
}

//*****************************************************************************
//
// The interrupt handler for the for Systick interrupt.
//
//*****************************************************************************
static int uartCounter = 0;
void SysTickIntHandler(void) {
	uartCounter++;

}


int main(void) {
	//FPUEnable();
	//FPULazyStackingEnable();
	//
	// Setup the system clock to run at 80 Mhz from PLL with crystal reference
	//
	SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
	//SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, RED_LED | BLUE_LED | GREEN_LED);
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GREEN_LED);
	GPIOPinWrite(GPIO_PORTF_BASE, RED_LED, 1 & 0xFF ? RED_LED : 0);
	GPIOPinWrite(GPIO_PORTF_BASE, BLUE_LED, 1 & 0xFF ? BLUE_LED : 0);
	GPIOPinWrite(GPIO_PORTF_BASE, GREEN_LED, 1 & 0xFF ? GREEN_LED : 0);
	GPIOPinWrite(GPIO_PORTF_BASE, BLUE_LED, 0 & 0xFF ? BLUE_LED : 0);

	//debug Console
	debugConsoleInit();

	//Display driver
	ILI9320Init();

	//Configuration
	configInit();


    tContext sContext;
    tRectangle sRect;
    GrContextInit(&sContext, &g_ILI9320);
    //FrameDraw(&sContext, "hello-widget");
#if 1
    //
    // Fill the top 24 rows of the screen with blue to create the banner.
    //
    sRect.i16XMin = 0;
    sRect.i16YMin = 0;
    sRect.i16XMax = GrContextDpyWidthGet(&sContext) - 1;
    sRect.i16YMax = 23;
    GrContextForegroundSet(&sContext, ClrDarkBlue);
    GrRectFill(&sContext, &sRect);

    //
    // Put a Red box around the banner.
    //
    GrContextForegroundSet(&sContext, ClrRed);
    GrRectDraw(&sContext, &sRect);

    // Put the application name in the middle of the banner.
    GrContextForegroundSet(&sContext, ClrYellowGreen);
    GrContextFontSet(&sContext, &g_sFontCm20);
    GrStringDrawCentered(&sContext, "Meteo Ubiad Stacja", -1,
                         GrContextDpyWidthGet(&sContext) / 2, 8, 0);
#endif
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sBackground);
    WidgetPaint(WIDGET_ROOT);
    WidgetAdd((tWidget *)&g_sPushBtn, (tWidget *)&g_sHello);

    PushButtonTextSet(&g_sPushBtn, "Hide Welcome");

    //
    // Repaint the pushbutton and all widgets beneath it (in this case,
    // the welcome message).
    //
    WidgetPaint((tWidget *)&g_sPushBtn);


	//touchScreenControler
	touchScreenInit();

	//wifi module
    //esp8266Init();


	//Enable all interrupts
	IntMasterEnable();

	// Enable the SysTick and its Interrupt.
	SysTickPeriodSet(SysCtlClockGet());
	SysTickIntEnable();
	SysTickEnable();

	while (1) {
		ConfigParameters* cfg1 = configGetCurrent();
		if(cfg1->touchScreenParams.isUpdated == 0){}
        // Process any messages in the widget message queue.
        WidgetMessageQueueProcess();
#if 1

		if(!ADS7843getIntPinState()) //if touch panel is being touched
		{
			ADS7843read();
			TouchPoint a;
			a = ADS7843getTouchedPoint();
			//debugConsolePrintf("RESULTS: x=%d, y=%d\n\r", a.x, a.y);
			GrContextForegroundSet(&sContext, ClrRed);
			GrCircleFill(&sContext, a.x, a.y, 3);
		}
		else
		{
			CalibCoefficients coeffs;
			ConfigParameters* cfg = configGetCurrent();
			if(cfg->touchScreenParams.isUpdated == 0)
			{
				performThreePointCalibration(&sContext, &coeffs);
				ADS7843setCalibrationCoefficients(&coeffs);
				cfg->touchScreenParams.calibCoeffs = coeffs;
				cfg->touchScreenParams.isUpdated = 0xFF;
				configSave();

				debugConsolePrintf("COEFFSa: a.x=%d, a.y=%d\n\r", coeffs.m_ax, coeffs.m_ay);
				debugConsolePrintf("COEFFSb: b.x=%d, b.y=%d\n\r", coeffs.m_bx, coeffs.m_by);
				debugConsolePrintf("COEFFSd: d.x=%d, d.y=%d\n\r", coeffs.m_dx, coeffs.m_dy);
			}
			else
			{
				ADS7843setCalibrationCoefficients(&configGetCurrent()->touchScreenParams.calibCoeffs);
			}
			//debugConsolePrintf("NOT_PUSHED \r");
		}

#endif
	}

}
