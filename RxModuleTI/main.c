#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/systick.h"
#include "driverlib/interrupt.h"
#include "ILI9320.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "utils/uartstdio.h"
#include "ugui.h"



static UG_GUI gui;

#define RGB2RGB565(c) (((((c & 0x00ff0000) >> 16) >> 3) << 11) | \
    ((((c & 0x0000ff00) >> 8) >> 2) << 5) | (((c & 0x000000ff) >> 0) >> 3 ))

static void pixelSet(UG_S16 x, UG_S16 y, UG_COLOR c)
{
    drawPixel(x, y, RGB2RGB565(c));
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
    init(_16_BIT, LANDSCAPE);
    UG_Init(&gui, pixelSet, 320, 240);
    UG_SelectGUI(&gui);
	UG_FontSelect( &FONT_12X20 );
	UG_SetForecolor(C_OLIVE);
	UG_SetBackcolor(C_WHITE);
	UG_PutString( 0, 40, (char*)"Hello World");
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









//*****************************************************************************
//
//! \addtogroup systick_examples_list
//! <h1>Systick Interrupt (systick_int)</h1>
//!
//! This example shows how to configure the SysTick and the SysTick
//! interrupt.
//!
//! This example uses the following peripherals and I/O signals.  You must
//! review these and change as needed for your own board:
//! - NONE
//!
//! The following UART signals are configured only for displaying console
//! messages for this example.  These are not required for operation of
//! Systick.
//! - UART0 peripheral
//! - GPIO Port A peripheral (for UART0 pins)
//! - UART0RX - PA0
//! - UART0TX - PA1
//!
//! This example uses the following interrupt handlers.  To use this example
//! in your own application you must add these interrupt handlers to your
//! vector table.
//! - SysTickIntHandler
//!
//
//*****************************************************************************

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
void
InitConsole(void)
{
    //
    // Enable GPIO port A which is used for UART0 pins.
    // TODO: change this to whichever GPIO port you are using.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    //
    // Configure the pin muxing for UART0 functions on port A0 and A1.
    // This step is not necessary if your part does not support pin muxing.
    // TODO: change this to select the port/pin you are using.
    //
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);

    //
    // Select the alternate (UART) function for these pins.
    // TODO: change this to select the port/pin you are using.
    //
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Initialize the UART for console I/O.
    //
    UARTStdioInit(0);
}

//*****************************************************************************
//
// The interrupt handler for the for Systick interrupt.
//
//*****************************************************************************
void
SysTickIntHandler(void)
{
    //
    // Update the Systick interrupt counter.
    //
    g_ulCounter++;
}

//*****************************************************************************
//
// Configure the SysTick and SysTick interrupt with a period of 1 second.
//
//*****************************************************************************

//*****************************************************************************
//
// Define pin to LED color mapping.
//
//*****************************************************************************

#define RED_LED   GPIO_PIN_1
#define BLUE_LED  GPIO_PIN_2
#define GREEN_LED GPIO_PIN_3

int main(void)
{
    //
    // Setup the system clock to run at 80 Mhz from PLL with crystal reference
    //
	SysCtlClockSet(SYSCTL_SYSDIV_2_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ);
    //SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
    //
    // Enable and configure the GPIO port for the LED operation.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, RED_LED|BLUE_LED|GREEN_LED);

    //init(_16_BIT, PORTRAIT);
    //init(_16_BIT, LANDSCAPE);
    gui_init();

    drawRoundRect(10, 10, 200, 260, BLUE);
    fillCircle(120, 120, 50, BLUE);
    InitConsole();
    unsigned long ulPrevCount = 0;
	//
	// Display the setup on the console.
	//
	UARTprintf("SysTick Firing Interrupt ->");
	UARTprintf("\n   Rate = 1sec\n\n");

	//
	// Initialize the interrupt counter.
	//
	g_ulCounter = 0;

	SysTickPeriodSet(SysCtlClockGet());

	//Enable all interrupts
	IntMasterEnable();

	// Enable the SysTick Interrupt.
	SysTickIntEnable();

	SysTickEnable();

    while(1)
    {

       //GPIOPinWrite(GPIO_PORTF_BASE, RED_LED|BLUE_LED|GREEN_LED, RED_LED);
        GPIOPinWrite(GPIO_PORTF_BASE, RED_LED|BLUE_LED|GREEN_LED, RED_LED);
        // Check to see if systick interrupt count changed, and if so then
        // print a message with the count.
        //
        if(ulPrevCount != g_ulCounter)
        {
            //
            // Print the interrupt counter.
            //
            UARTprintf("Number of interrupts: %d\r", g_ulCounter);
            GPIOPinWrite(GPIO_PORTF_BASE, RED_LED|BLUE_LED|GREEN_LED, BLUE_LED);
            ulPrevCount = g_ulCounter;
        }
    }
}
