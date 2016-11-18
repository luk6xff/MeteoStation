#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/systick.h"
#include "driverlib/interrupt.h"
#include "driverlib/uart.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "inc/hw_types.h"
#include "utils/uartstdio.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "grlib/pushbutton.h"
#include "grlib/checkbox.h"
#include "grlib/container.h"
#include "grlib/radiobutton.h"
#include "grlib/slider.h"
#include "grlib/frame.h"
#include "utils/ustdlib.h"
#include "utils/sine.h"
#include "driverlib/fpu.h"
#include "ILI9320.h"
#include "ILI9320_driver.h"
#include "ads7843.h"
#include "3PointCalibration.h"

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







void UART5IntHandler(void) {
	unsigned long ulStatus;

	//
	// Get the interrrupt status.
	//
	ulStatus = UARTIntStatus(UART5_BASE, true);

	//
	// Clear the asserted interrupts.
	//
	UARTIntClear(UART5_BASE, ulStatus);

	//
	// Loop while there are characters in the receive FIFO.
	//
	while (UARTCharsAvail(UART5_BASE)) {
		// Read the next character from the UART and write it to the console.

		//UARTCharPutNonBlocking(UART5_BASE, UARTCharGetNonBlocking(UART5_BASE));
		UARTprintf("%c", UARTCharGetNonBlocking(UART5_BASE));
	}
}

//*****************************************************************************
//
// Send a string to the UART.
//
//*****************************************************************************
static void UARTSend(const unsigned char *pucBuffer, unsigned long ulCount) {
	// Loop while there are more characters to send.
	while (ulCount--) {
		// Write the next character to the UART.
		UARTCharPutNonBlocking(UART5_BASE, *pucBuffer++);
	}
}

//! - UART5 peripheral
//! - GPIO Port E peripheral (for UART5 pins)
//! - UART1RX - PE4
//! - UART1TX - PE5

static void uartESP8266Setup(void) {
	//
	// Set the clocking to run directly from the crystal.
	//
	//SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
	//SYSCTL_XTAL_16MHZ);

	//
	// Enable the GPIO port that is used for the on-board LED.
	//
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

	//
	// Enable the GPIO pins for the LED (PF2).
	//
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, RED_LED | BLUE_LED | GREEN_LED);

	//
	// Enable the peripherals used by this example.
	//
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART5);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);

	//
	// Enable processor interrupts.
	//
	IntMasterEnable();

	//
	// Set GPIO PE4 and PE5 as UART pins.
	//
	GPIOPinConfigure(GPIO_PE4_U5RX);
	GPIOPinConfigure(GPIO_PE5_U5TX);
	GPIOPinTypeUART(GPIO_PORTE_BASE, GPIO_PIN_4 | GPIO_PIN_5);

	//
	// Configure the UART for 115,200, 8-N-1 operation.
	//
	UARTConfigSetExpClk(UART5_BASE, SysCtlClockGet(), 115200,
			(UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
			UART_CONFIG_PAR_NONE));

	//
	// Enable the UART interrupt.
	//
	IntEnable(INT_UART5);
	UARTIntEnable(UART5_BASE, UART_INT_RX | UART_INT_RT);

	//
	// Prompt for text to be entered.
	//
	UARTSend((unsigned char *) "AT", 2);

}

//! - UART0 peripheral - Console UART
//! - GPIO Port A peripheral (for UART0 pins)
//! - UART0RX - PA0
//! - UART0TX - PA1
//*****************************************************************************
//
// Counter to count the number of interrupts that have been called.
//
//*****************************************************************************
unsigned long g_ulCounter = 0;

//*****************************************************************************
//
// This function sets up UART0 to be used for a console to display information
// as the example is running.
//
//*****************************************************************************
void InitConsole(void) {
	// Enable GPIO port A which is used for UART0 pins.
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

	GPIOPinConfigure(GPIO_PA0_U0RX);
	GPIOPinConfigure(GPIO_PA1_U0TX);

	GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	// Initialize the UART for console I/O.
	UARTStdioInit(0);
}

//*****************************************************************************
//
// The interrupt handler for the for Systick interrupt.
//
//*****************************************************************************
static int uartCounter = 0;
void SysTickIntHandler(void) {
	g_ulCounter++;
	uartCounter++;

}


int main(void) {
	FPUEnable();
	FPULazyStackingEnable();
	//
	// Setup the system clock to run at 80 Mhz from PLL with crystal reference
	//
	SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
	//SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
	unsigned long ulPrevCount = 0;
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, RED_LED | BLUE_LED | GREEN_LED);
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GREEN_LED);
	GPIOPinWrite(GPIO_PORTF_BASE, RED_LED, 1 & 0xFF ? RED_LED : 0);
	GPIOPinWrite(GPIO_PORTF_BASE, BLUE_LED, 1 & 0xFF ? BLUE_LED : 0);
	GPIOPinWrite(GPIO_PORTF_BASE, GREEN_LED, 1 & 0xFF ? GREEN_LED : 0);
	GPIOPinWrite(GPIO_PORTF_BASE, BLUE_LED, 0 & 0xFF ? BLUE_LED : 0);
	ILI9320Init();

	//ADS7843
	ADS7843init();

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

    uartESP8266Setup();
	InitConsole();
	//UARTprintf("SysTick Firing Interrupt ->");
	//UARTprintf("\n   Rate = 1sec\n\n");

	//Enable all interrupts
	IntMasterEnable();

	// Enable the SysTick and its Interrupt.
	g_ulCounter = 0;
	//SysTickPeriodSet(SysCtlClockGet());
	//SysTickIntEnable();
	//SysTickEnable();

	while (1) {
        // Process any messages in the widget message queue.
        WidgetMessageQueueProcess();
#if 1
		// Check to see if systick interrupt count changed, and if so then
		// print a message with the count.
		if (ulPrevCount != g_ulCounter) {
			//
			// Print the interrupt counter.
			//
			UARTprintf("Number of interrupts: %d\r", g_ulCounter);
			GPIOPinWrite(GPIO_PORTF_BASE, RED_LED | BLUE_LED | GREEN_LED,
					BLUE_LED);
			ulPrevCount = g_ulCounter;
		}

		if (!(uartCounter % 50)) {
			UARTSend((unsigned char *) "AT\r\n", 4);
		}

		if(!ADS7843getIrqPinState()) //if touch panel is being touched
		{
			ADS7843read();
			TouchPoint a;
			a = ADS7843getTouchedPoint();
			UARTprintf("RESULTS: x=%d, y=%d\n\r", a.x, a.y);
			GrContextForegroundSet(&sContext, ClrRed);
			GrCircleFill(&sContext, a.x, a.y, 3);
		}
		else
		{
			static int once = 0;
			CalibCoefficients coefs;
			if(!once)
			{
				performThreePointCalibration(&sContext, &coefs);
				ADS7843setCalibrationCoefficients(&coefs);
				UARTprintf("COEFFSa: a.x=%d, a.y=%d\n\r", coefs.m_ax, coefs.m_ay);
				UARTprintf("COEFFSb: b.x=%d, b.y=%d\n\r", coefs.m_bx, coefs.m_by);
				UARTprintf("COEFFSd: d.x=%d, d.y=%d\n\r", coefs.m_dx, coefs.m_dy);
				once++;
			}
			//UARTprintf("NOT_PUSHED \r");
		}

#endif
	}

}
