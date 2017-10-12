/*
 * spiCommon.c
 *
 *  Created on: 12 paü 2017
 *      Author: igbt6
 */

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"

#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/ssi.h"

#include "spiCommon.h"

static bool spiInitialized = false;

// Configure the SoftSSI module.  The size of the FIFO buffers can be
// Module SSI0, pins are assigned as follows:
//      PA2 - SoftSSICLK
//      SoftSSIFss  --will be handled manually
//      PA4 - SoftSSIRx
//      PA5 - SoftSSITx
void spiCommonInit(void)
{

	unsigned long rxBuf[1];
	SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);

	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

	GPIOPinConfigure(GPIO_PA2_SSI0CLK);
	GPIOPinConfigure(GPIO_PA4_SSI0RX);
	GPIOPinConfigure(GPIO_PA5_SSI0TX);

	GPIOPinTypeSSI(GPIO_PORTA_BASE, GPIO_PIN_5 | GPIO_PIN_4 | GPIO_PIN_2);

	// Configure and enable the SSI port for SPI master mode.  Use SSI0,
	// system clock supply, idle clock level low and active low clock in
	// freescale SPI mode, master mode, 1MHz SSI frequency, and 8-bit data.
	SSIConfigSetExpClk(SSI0_BASE, SysCtlClockGet(), SSI_FRF_MOTO_MODE_0,
					   SSI_MODE_MASTER, 100000, 8);
	// Enable the SSI0 module.
	SSIEnable(SSI0_BASE);

	//flush the receives fifos
	while (SSIDataGetNonBlocking(SSI0_BASE, &rxBuf[0]))
	{
	}
	spiInitialized = true;
}

bool spiCommonIsSpiInitialized(void)
{
	return spiInitialized;
}

bool spiCommonIsSpiBusy(void)
{
	return SSIBusy(SSI0_BASE);
}
