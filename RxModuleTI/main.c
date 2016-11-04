#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/systick.h"
#include "driverlib/interrupt.h"
#include "driverlib/uart.h"
#include "ILI9320.h"
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
#include "utils/ustdlib.h"
#include "utils/sine.h"

#define RED_LED   GPIO_PIN_1
#define BLUE_LED  GPIO_PIN_2
#define GREEN_LED GPIO_PIN_3

//*****************************************************************************
//
//! The display structure that describes the driver for the used LCD with
//!  with an ILI9320 controller.
//
//*****************************************************************************

#define DPYCOLORTRANSLATE(c)    ((((c) & 0x00f80000) >> 8) |                  \
                                 (((c) & 0x0000fc00) >> 5) |                  \
                                 (((c) & 0x000000f8) >> 3))

static void ILI9320PixelDraw(void *pvDisplayData, int32_t i32X, int32_t i32Y,
		uint32_t ui32Value) {
	drawPixel(i32X, i32Y, ui32Value);
}

static void ILI9320PixelDrawMultiple(void *pvDisplayData, int32_t i32X,
                                          int32_t i32Y, int32_t i32X0,
                                          int32_t i32Count, int32_t i32BPP,
                                          const uint8_t *pui8Data,
                                          const uint8_t *pui8Palette)
{

}

static void ILI9320LineDrawH(void *pvDisplayData, int32_t i32X1, int32_t i32X2,
		int32_t i32Y, uint32_t ui32Value) {
	drawHLine(i32X1, i32Y, i32X2 -i32X1, ui32Value );
}

static void ILI9320LineDrawV(void *pvDisplayData, int32_t i32X,
                                  int32_t i32Y1, int32_t i32Y2,
                                  uint32_t ui32Value)
{
	drawVLine(i32X, i32Y1, i32Y2 -i32Y1, ui32Value );
}

static void ILI9320RectFill(void *pvDisplayData, const tRectangle *psRect,
                                 uint32_t ui32Value)
{
	fillRect(psRect->i16XMin, psRect->i16YMin, psRect->i16XMax, psRect->i16YMax,  ui32Value);
}

static uint32_t ILI9320ColorTranslate(void *pvDisplayData, uint32_t ui32Value)
{
    //
    // Translate from a 24-bit RGB color to a 5-6-5 RGB color.
    //
    return(DPYCOLORTRANSLATE(ui32Value));
}

static void ILI9320Flush(void *pvDisplayData)
{
    // There is nothing to be done.
}

const tDisplay m_ILI9320 =
{
		sizeof(tDisplay), 0,
#if defined(PORTRAIT) || defined(PORTRAIT_FLIP)
		240,
		320,
#else
		320, 240,
#endif
		ILI9320PixelDraw, //Kentec320x240x16_SSD2119PixelDraw,
		ILI9320PixelDrawMultiple, //Kentec320x240x16_SSD2119PixelDrawMultiple,
		ILI9320LineDrawH, //Kentec320x240x16_SSD2119LineDrawH,
		ILI9320LineDrawV, //Kentec320x240x16_SSD2119LineDrawV,
		ILI9320RectFill, //Kentec320x240x16_SSD2119RectFill,
		ILI9320ColorTranslate, //Kentec320x240x16_SSD2119ColorTranslate,
		ILI9320Flush //Kentec320x240x16_SSD2119Flush
};
//*****************************************************************************
//
// The first panel, which contains introductory text explaining the
// application.
//
//*****************************************************************************
#define X_OFFSET                8
#define Y_OFFSET                24
extern tCanvasWidget g_psPanels[];
//*****************************************************************************
//
// Handles paint requests for the introduction canvas widget.
//
//*****************************************************************************
void OnIntroPaint(tWidget *psWidget, tContext *psContext) {
	//
	// Display the introduction text in the canvas.
	//
	GrContextFontSet(psContext, g_psFontCm16);
	GrContextForegroundSet(psContext, ClrSilver);
	GrStringDraw(psContext, "This application demonstrates the ", -1, 10, 30,
			0);
	GrStringDraw(psContext, "TivaWare Graphics Library.", -1, 10, (30 + 16), 0);
	GrStringDraw(psContext, "Each panel shows a different feature of", -1, 10,
			(30 + (16 * 2)), 0);
	GrStringDraw(psContext, "the graphics library. Widgets on the panels", -1,
			10, (30 + (16 * 3)), 0);
	GrStringDraw(psContext, "are fully operational; pressing them will", -1, 10,
			(30 + (16 * 4)), 0);
	GrStringDraw(psContext, "result in visible feedback of some kind.", -1, 10,
			(30 + (16 * 5)), 0);
	GrStringDraw(psContext, "Press the + and - buttons at the bottom", -1, 10,
			(30 + (16 * 6)), 0);
	GrStringDraw(psContext, "of the screen to move between the panels.", -1, 10,
			(30 + (16 * 7)), 0);
}
Canvas(g_sIntroduction, g_psPanels, 0, 0, &m_ILI9320, X_OFFSET, Y_OFFSET,
		(320 - (X_OFFSET*2)), 158, CANVAS_STYLE_APP_DRAWN, 0, 0, 0, 0, 0, 0,
		OnIntroPaint);

tCanvasWidget g_psPanels[] =
{
    CanvasStruct(0, 0, &g_sIntroduction, &m_ILI9320,
                 X_OFFSET, Y_OFFSET, (320 - (X_OFFSET*2)), 158,
                 CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0)

};

char *g_pcPanelNames[] =
{
    "     Introduction     "
};

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
	//
	// Setup the system clock to run at 80 Mhz from PLL with crystal reference
	//
	SysCtlClockSet(
			SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN
					| SYSCTL_XTAL_16MHZ);
	//SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
	unsigned long ulPrevCount = 0;
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, RED_LED | BLUE_LED | GREEN_LED);
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GREEN_LED);
	GPIOPinWrite(GPIO_PORTF_BASE, RED_LED, 1 & 0xFF ? RED_LED : 0);
	GPIOPinWrite(GPIO_PORTF_BASE, BLUE_LED, 1 & 0xFF ? BLUE_LED : 0);
	GPIOPinWrite(GPIO_PORTF_BASE, GREEN_LED, 1 & 0xFF ? GREEN_LED : 0);
	GPIOPinWrite(GPIO_PORTF_BASE, BLUE_LED, 0 & 0xFF ? BLUE_LED : 0);
	//init(_16_BIT, PORTRAIT);
	init(_16_BIT, LANDSCAPE);
	//gui_init();

	drawHLine(0, 0, 230, BLUE);
	uartESP8266Setup();

    tContext sContext;
    GrContextInit(&sContext, &m_ILI9320);

    //
    // Draw the application frame.
    //
    //FrameDraw(&sContext, "grlib-demo");
    //WidgetAdd(WIDGET_ROOT, (tWidget *)(g_psPanels));
    //WidgetPaint((tWidget *)(g_psPanels + 0));
    WidgetPaint((tWidget *)(g_psPanels));

	InitConsole();
	UARTprintf("SysTick Firing Interrupt ->");
	UARTprintf("\n   Rate = 1sec\n\n");
	UARTprintf("REG: 0x%x", ReadRegister(0x0000));

	g_ulCounter = 0;
	SysTickPeriodSet(SysCtlClockGet());

	//Enable all interrupts
	IntMasterEnable();

	// Enable the SysTick Interrupt.
	SysTickIntEnable();

	SysTickEnable();
	while (1) {

		GPIOPinWrite(GPIO_PORTF_BASE, RED_LED | BLUE_LED | GREEN_LED, RED_LED);
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
	}
}
