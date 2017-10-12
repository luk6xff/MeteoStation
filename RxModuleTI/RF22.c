// RF22.c
//
// Copyright (C) 2011 Mike McCauley
// $Id: RF22.cpp,v 1.17 2013/02/06 21:33:56 mikem Exp mikem $
// ported to mbed by Karl Zweimueller
// modified by Lukasz Uszko luszko@op.pl








/**
 * @file      [file name].c
 * @authors   [author]
 * @copyright [copy write holder]
 *
 * @brief [description]
 */
/*******************************************************************************
* Includes
*******************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "spiCommon.h"

#include "RF22.h"


/*******************************************************************************
* Defines
*******************************************************************************/
// The acknowledgement bit in the FLAGS
#define RF22_FLAGS_ACK 0x80

/*******************************************************************************
* Constants
*******************************************************************************/
// These are indexed by the values of ModemConfigChoice
// Canned modem configurations generated with
// http://www.hoperf.com/upload/rf/RF22B%2023B%2031B%2042B%2043B%20Register%20Settings_RevB1-v5.xls
// Stored in flash (program) memory to save SRAM
static const RF22_ModemConfig MODEM_CONFIG_TABLE[] = {
    { 0x2b, 0x03, 0xf4, 0x20, 0x41, 0x89, 0x00, 0x36, 0x40, 0x0a, 0x1d, 0x80, 0x60, 0x10, 0x62, 0x2c, 0x00, 0x08 }, // Unmodulated carrier
    { 0x2b, 0x03, 0xf4, 0x20, 0x41, 0x89, 0x00, 0x36, 0x40, 0x0a, 0x1d, 0x80, 0x60, 0x10, 0x62, 0x2c, 0x33, 0x08 }, // FSK, PN9 random modulation, 2, 5

    // All the following enable FIFO with reg 71
    //  1c,   1f,   20,   21,   22,   23,   24,   25,   2c,   2d,   2e,   58,   69,   6e,   6f,   70,   71,   72
    // FSK, No Manchester, Max Rb err <1%, Xtal Tol 20ppm
    { 0x2b, 0x03, 0xf4, 0x20, 0x41, 0x89, 0x00, 0x36, 0x40, 0x0a, 0x1d, 0x80, 0x60, 0x10, 0x62, 0x2c, 0x22, 0x08 }, // 2, 5
    { 0x1b, 0x03, 0x41, 0x60, 0x27, 0x52, 0x00, 0x07, 0x40, 0x0a, 0x1e, 0x80, 0x60, 0x13, 0xa9, 0x2c, 0x22, 0x3a }, // 2.4, 36
    { 0x1d, 0x03, 0xa1, 0x20, 0x4e, 0xa5, 0x00, 0x13, 0x40, 0x0a, 0x1e, 0x80, 0x60, 0x27, 0x52, 0x2c, 0x22, 0x48 }, // 4.8, 45
    { 0x1e, 0x03, 0xd0, 0x00, 0x9d, 0x49, 0x00, 0x45, 0x40, 0x0a, 0x20, 0x80, 0x60, 0x4e, 0xa5, 0x2c, 0x22, 0x48 }, // 9.6, 45
    { 0x2b, 0x03, 0x34, 0x02, 0x75, 0x25, 0x07, 0xff, 0x40, 0x0a, 0x1b, 0x80, 0x60, 0x9d, 0x49, 0x2c, 0x22, 0x0f }, // 19.2, 9.6
    { 0x02, 0x03, 0x68, 0x01, 0x3a, 0x93, 0x04, 0xd5, 0x40, 0x0a, 0x1e, 0x80, 0x60, 0x09, 0xd5, 0x0c, 0x22, 0x1f }, // 38.4, 19.6
    { 0x06, 0x03, 0x45, 0x01, 0xd7, 0xdc, 0x07, 0x6e, 0x40, 0x0a, 0x2d, 0x80, 0x60, 0x0e, 0xbf, 0x0c, 0x22, 0x2e }, // 57.6. 28.8
    { 0x8a, 0x03, 0x60, 0x01, 0x55, 0x55, 0x02, 0xad, 0x40, 0x0a, 0x50, 0x80, 0x60, 0x20, 0x00, 0x0c, 0x22, 0xc8 }, // 125, 125

    // GFSK, No Manchester, Max Rb err <1%, Xtal Tol 20ppm
    // These differ from FSK only in register 71, for the modulation type
    { 0x2b, 0x03, 0xf4, 0x20, 0x41, 0x89, 0x00, 0x36, 0x40, 0x0a, 0x1d, 0x80, 0x60, 0x10, 0x62, 0x2c, 0x23, 0x08 }, // 2, 5
    { 0x1b, 0x03, 0x41, 0x60, 0x27, 0x52, 0x00, 0x07, 0x40, 0x0a, 0x1e, 0x80, 0x60, 0x13, 0xa9, 0x2c, 0x23, 0x3a }, // 2.4, 36
    { 0x1d, 0x03, 0xa1, 0x20, 0x4e, 0xa5, 0x00, 0x13, 0x40, 0x0a, 0x1e, 0x80, 0x60, 0x27, 0x52, 0x2c, 0x23, 0x48 }, // 4.8, 45
    { 0x1e, 0x03, 0xd0, 0x00, 0x9d, 0x49, 0x00, 0x45, 0x40, 0x0a, 0x20, 0x80, 0x60, 0x4e, 0xa5, 0x2c, 0x23, 0x48 }, // 9.6, 45
    { 0x2b, 0x03, 0x34, 0x02, 0x75, 0x25, 0x07, 0xff, 0x40, 0x0a, 0x1b, 0x80, 0x60, 0x9d, 0x49, 0x2c, 0x23, 0x0f }, // 19.2, 9.6
    { 0x02, 0x03, 0x68, 0x01, 0x3a, 0x93, 0x04, 0xd5, 0x40, 0x0a, 0x1e, 0x80, 0x60, 0x09, 0xd5, 0x0c, 0x23, 0x1f }, // 38.4, 19.6
    { 0x06, 0x03, 0x45, 0x01, 0xd7, 0xdc, 0x07, 0x6e, 0x40, 0x0a, 0x2d, 0x80, 0x60, 0x0e, 0xbf, 0x0c, 0x23, 0x2e }, // 57.6. 28.8
    { 0x8a, 0x03, 0x60, 0x01, 0x55, 0x55, 0x02, 0xad, 0x40, 0x0a, 0x50, 0x80, 0x60, 0x20, 0x00, 0x0c, 0x23, 0xc8 }, // 125, 125

    // OOK, No Manchester, Max Rb err <1%, Xtal Tol 20ppm
    { 0x51, 0x03, 0x68, 0x00, 0x3a, 0x93, 0x01, 0x3d, 0x2c, 0x11, 0x28, 0x80, 0x60, 0x09, 0xd5, 0x2c, 0x21, 0x08 }, // 1.2, 75
    { 0xc8, 0x03, 0x39, 0x20, 0x68, 0xdc, 0x00, 0x6b, 0x2a, 0x08, 0x2a, 0x80, 0x60, 0x13, 0xa9, 0x2c, 0x21, 0x08 }, // 2.4, 335
    { 0xc8, 0x03, 0x9c, 0x00, 0xd1, 0xb7, 0x00, 0xd4, 0x29, 0x04, 0x29, 0x80, 0x60, 0x27, 0x52, 0x2c, 0x21, 0x08 }, // 4.8, 335
    { 0xb8, 0x03, 0x9c, 0x00, 0xd1, 0xb7, 0x00, 0xd4, 0x28, 0x82, 0x29, 0x80, 0x60, 0x4e, 0xa5, 0x2c, 0x21, 0x08 }, // 9.6, 335
    { 0xa8, 0x03, 0x9c, 0x00, 0xd1, 0xb7, 0x00, 0xd4, 0x28, 0x41, 0x29, 0x80, 0x60, 0x9d, 0x49, 0x2c, 0x21, 0x08 }, // 19.2, 335
    { 0x98, 0x03, 0x9c, 0x00, 0xd1, 0xb7, 0x00, 0xd4, 0x28, 0x20, 0x29, 0x80, 0x60, 0x09, 0xd5, 0x0c, 0x21, 0x08 }, // 38.4, 335
    { 0x98, 0x03, 0x96, 0x00, 0xda, 0x74, 0x00, 0xdc, 0x28, 0x1f, 0x29, 0x80, 0x60, 0x0a, 0x3d, 0x0c, 0x21, 0x08 }, // 40, 335
};
/*******************************************************************************
* Local Types and Typedefs
*******************************************************************************/

/*******************************************************************************
* Global Variables
*******************************************************************************/

/*******************************************************************************
* Static Function Prototypes
*******************************************************************************/

/*******************************************************************************
* Static Variables
*******************************************************************************/
static volatile uint8_t    _mode; // One of RF22_MODE_*

static uint8_t             _idleMode;
//NOT USED                  _shutdownPin; // SDN should be = 0 in all modes except Shutdown mode. When SDN =1 the chip will be completely shutdown and the contents of the registers will be lost
static uint8_t             _deviceType;

// These volatile members may get changed in the interrupt service routine
static volatile uint8_t    _bufLen;
static uint8_t             _buf[RF22_MAX_MESSAGE_LEN];

static volatile bool       _rxBufValid;

static volatile bool       _txPacketSent;
static volatile uint8_t    _txBufSentIndex;

static volatile uint16_t   _rxBad;
static volatile uint16_t   _rxGood;
static volatile uint16_t   _txGood;

static volatile uint8_t    _lastRssi;

/// Count of retransmissions we have had to send
static uint16_t _retransmissions;

/// The last sequence number to be used
/// Defaults to 0
static uint8_t _lastSequenceNumber;

// Retransmit timeout (milliseconds)
/// Defaults to 200
static uint16_t _timeout;

// Retries (0 means one try only)
/// Defaults to 3
static uint8_t _retries;

/// Array of the last seen sequence number indexed by node address that sent it
/// It is used for duplicate detection. Duplicated messages are re-acknowledged when received
/// (this is generally due to lost ACKs, causing the sender to retransmit, even though we have already
/// received that message)
static uint8_t _seenIds[256];


/// The address of this node. Defaults to 0.
static uint8_t _thisAddress;

/// Current timer time in seconds
static volatile uint32_t _time_counter = 0;

/*******************************************************************************
* Hardware dependent stuff (SPI, Timer, etc)
*******************************************************************************/
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"

#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/timer.h"

////////////////////////////////TIMER////////////////////////////////
//Configures Timer4A as a 32-bit periodic timer [HW Dependent]
static void RF22_TimerInit()
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER4);

    TimerConfigure(TIMER4_BASE, TIMER_CFG_32_BIT_PER_UP);
    // Set the Timer4A load value to 1ms.
    TimerLoadSet(TIMER4_BASE, TIMER_A, SysCtlClockGet() / 1000); //1 [ms]

    // Configure the Timer interrupt for timer timeout.
    TimerIntEnable(TIMER4_BASE, TIMER_TIMA_TIMEOUT);

    // Set Low interrupt priority for Timer
    IntPrioritySet(INT_TIMER4A, 3);

    // Enable the Timer interrupt on the processor (NVIC).
    IntEnable(INT_TIMER4A);

    _time_counter = 0;

    // Enable Timer.
    TimerEnable(TIMER4_BASE, TIMER_A);
}

static void RF22_TimerWait_ms(uint32_t ms)
{
	uint32_t start = _time_counter;
	while((_time_counter - start) < ms)
	{
	}
}

//Timer4A interrupt handler
void RF22_Timer4AIntHandler(void)
{
	TimerIntClear(TIMER4_BASE, TIMER_TIMA_TIMEOUT);
	++_time_counter;
}

/////////////////////////////SPI////////////////////////////////
#include "driverlib/ssi.h"
#include "driverlib/sysctl.h"

//------SSI0 RFM_22 CS_PIN - PF0 as stated in MeteoStationDocumentation.odt---------
#define RFM_22_PIN_CS      			GPIO_PIN_0
#define RFM_22_PORT_CS     			GPIO_PORTF_BASE
#define RFM_22_PORT_CS_CLOCK()		SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF)
#define RFM_22_CS_OUTPUT()  	    GPIOPinTypeGPIOOutput(RFM_22_PORT_CS, RFM_22_PIN_CS)
#define RFM_22_CS_HIGH()  			GPIOPinWrite(RFM_22_PORT_CS, RFM_22_PIN_CS, RFM_22_PIN_CS)
#define RFM_22_CS_LOW()  			GPIOPinWrite(RFM_22_PORT_CS, RFM_22_PIN_CS, 0)

static void spiInit(void)
{
	if (!spiCommonIsSpiInitialized())
	{
		while(1);
	}
	// Initialise the slave select pin
	RFM_22_PORT_CS_CLOCK();
	RFM_22_CS_OUTPUT(); //SSI0FSS will be handled manually
    RFM_22_CS_HIGH();
}

static uint8_t spiWriteReadByte(uint8_t val)
{
	uint8_t retVal = 0xFF; // ivalid value
	SSIDataPut(SSI0_BASE, val);
	while (SSIBusy(SSI0_BASE))
	{}
	SSIDataGet(SSI0_BASE, &retVal);
	return retVal;
}
//------------------------------------------------------------------------
/// Reads a single register from the RF22
/// \param[in] reg Register number, one of RF22_REG_*
/// \return The value of the register
static uint8_t spiRead(uint8_t reg)
{
    DISABLE_ALL_INTERRUPTS();    // Disable Interrupts
    RFM_22_CS_LOW();
    spiWriteReadByte(reg & ~RF22_SPI_WRITE_MASK); // Send the address with the write mask off
    uint8_t val = spiWriteReadByte(0); // The written value is ignored, reg value is read
    RFM_22_CS_HIGH();
    ENABLE_ALL_INTERRUPTS();     // Enable Interrupts
    return val;
}

//------------------------------------------------------------------------
/// Writes a single byte to the RF22
/// \param[in] reg Register number, one of RF22_REG_*
/// \param[in] val The value to write
static void spiWrite(uint8_t reg, uint8_t val)
{
	DISABLE_ALL_INTERRUPTS();
    RFM_22_CS_LOW();
    spiWriteReadByte(reg | RF22_SPI_WRITE_MASK); // Send the address with the write mask on
    spiWriteReadByte(val); // New value follows
    RFM_22_CS_HIGH();
    ENABLE_ALL_INTERRUPTS();
}

//------------------------------------------------------------------------
/// Reads a number of consecutive registers from the RF22 using burst read mode
/// \param[in] reg Register number of the first register, one of RF22_REG_*
/// \param[in] dest Array to write the register values to. Must be at least len bytes
/// \param[in] len Number of bytes to read
static void spiBurstRead(uint8_t reg, uint8_t* dest, uint8_t len)
{
    RFM_22_CS_LOW();
    spiWriteReadByte(reg & ~RF22_SPI_WRITE_MASK); // Send the start address with the write mask off
    while (len--)
    {
        *dest++ = spiWriteReadByte(0);
    }
    RFM_22_CS_HIGH();
}

//------------------------------------------------------------------------
/// Write a number of consecutive registers using burst write mode
/// \param[in] reg Register number of the first register, one of RF22_REG_*
/// \param[in] src Array of new register values to write. Must be at least len bytes
/// \param[in] len Number of bytes to write
static void spiBurstWrite(uint8_t reg, const uint8_t* src, uint8_t len)
{
    RFM_22_CS_LOW();
    spiWriteReadByte(reg | RF22_SPI_WRITE_MASK); // Send the start address with the write mask on
    while (len--)
    {
    	spiWriteReadByte(*src++);
    }
    RFM_22_CS_HIGH();
}

/////////////////////////////RFM_23 Interrupt Pin////////////////////////////////
//Configures interrupt pin supported for RF22
//------------------INT_IRQ PB_6 - as stated in MeteoStationDocumentation.odt----------------------
#define RFM_22_PIN_INT      			GPIO_PIN_6
#define RFM_22_PORT_INT     			GPIO_PORTB_BASE
#define RFM_22_INT_INTERRUPT_PORT     	INT_GPIOB
#define RFM_22_PORT_INT_CLOCK()		    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB)
#define RFM_22_INT_INPUT()  			GPIOPinTypeGPIOInput(RFM_22_PORT_INT, RFM_22_PIN_INT);  \
										GPIOPadConfigSet(RFM_22_PORT_INT, RFM_22_PIN_INT, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);  // Enable weak pullup resistor for the pin
#define RFM_22_INT_CONFIG_AS_FALLING()  GPIOIntTypeSet(RFM_22_PORT_INT, RFM_22_PIN_INT, GPIO_FALLING_EDGE)
#define RFM_22_INT_CONFIG_AS_RISING()   GPIOIntTypeSet(RFM_22_PORT_INT, RFM_22_PIN_INT, GPIO_RISING_EDGE)
#define RFM_22_INT_INTERRUPT_ENABLE() 	GPIOPinIntEnable(RFM_22_PORT_INT, RFM_22_PIN_INT); IntEnable(RFM_22_INT_INTERRUPT_PORT);
#define RFM_22_INT_INTERRUPT_DISABLE()  GPIOPinIntDisable(RFM_22_PORT_INT, RFM_22_PIN_INT); IntDisable(RFM_22_INT_INTERRUPT_PORT);
#define RFM_22_GET_INT_PIN() 			GPIOPinRead(RFM_22_PORT_INT, RFM_22_PIN_INT)

void RF22_PinIntHandler(void)
{
	GPIOPinIntClear(RFM_22_PORT_INT, RFM_22_PIN_INT);

	if(GPIOPinIntStatus(RFM_22_PORT_INT, false) & RFM_22_PIN_INT)
	{
		RF22_isr0();
	}
}

static void RF22_IntPinInit()
{
	RFM_22_PORT_INT_CLOCK();
	RFM_22_INT_INPUT();
	RFM_22_INT_CONFIG_AS_FALLING();
	GPIOPortIntRegister(RFM_22_PORT_INT, RF22_PinIntHandler);
	//ENABLE_ALL_INTERRUPTS();
	RFM_22_INT_INTERRUPT_ENABLE();
}

/*******************************************************************************
* Functions definitions
*******************************************************************************/

//------------------------------------------------------------------------
bool RF22_init()
{
    // Wait for RF22 POR (up to 16msec)
    //delay(16);
    _idleMode = RF22_XTON; // Default idle state is READY mode
    _mode = RF22_MODE_IDLE; // We start up in idle mode
    _rxGood = 0;
    _rxBad = 0;
    _txGood = 0;
    //TODO _shutdownPin= 0;  //power on - not used, connected to GND


    RF22_TimerWait_ms(16);

    spiInit();
    RF22_TimerInit();

    RF22_TimerWait_ms(100);

    // Software reset the device
    RF22_reset();

    // Get the device type and check it
    // This also tests whether we are really connected to a device
    _deviceType = spiRead(RF22_REG_00_DEVICE_TYPE);
    if (_deviceType != RF22_DEVICE_TYPE_RX_TRX
        && _deviceType != RF22_DEVICE_TYPE_TX)
    {
        return false;
    }

    // Set up interrupt handler
    RF22_IntPinInit();

    RF22_clearTxBuf();
    RF22_clearRxBuf();

    // Most of these are the POR default
    spiWrite(RF22_REG_7D_TX_FIFO_CONTROL2, RF22_TXFFAEM_THRESHOLD);
    spiWrite(RF22_REG_7E_RX_FIFO_CONTROL,  RF22_RXFFAFULL_THRESHOLD);
    spiWrite(RF22_REG_30_DATA_ACCESS_CONTROL, RF22_ENPACRX | RF22_ENPACTX | RF22_ENCRC | RF22_CRC_CRC_16_IBM);
    // Configure the message headers
    // Here we set up the standard packet format for use by the RF22 library
    // 8 nibbles preamble
    // 2 SYNC words 2d, d4
    // Header length 4 (to, from, id, flags)
    // 1 octet of data length (0 to 255)
    // 0 to 255 octets data
    // 2 CRC octets as CRC16(IBM), computed on the header, length and data
    // On reception the to address is check for validity against RF22_REG_3F_CHECK_HEADER3
    // or the broadcast address of 0xff
    // If no changes are made after this, the transmitted
    // to address will be 0xff, the from address will be 0xff
    // and all such messages will be accepted. This permits the out-of the box
    // RF22 config to act as an unaddresed, unreliable datagram service
    spiWrite(RF22_REG_32_HEADER_CONTROL1, RF22_BCEN_HEADER3 | RF22_HDCH_HEADER3);
    spiWrite(RF22_REG_33_HEADER_CONTROL2, RF22_HDLEN_4 | RF22_SYNCLEN_2);
    RF22_setPreambleLength(8);
    uint8_t syncwords[] = { 0x2d, 0xd4 };
    RF22_setSyncWords(syncwords, sizeof(syncwords));
    RF22_setPromiscuous(false);
    // Check the TO header against RF22_DEFAULT_NODE_ADDRESS
    spiWrite(RF22_REG_3F_CHECK_HEADER3, RF22_DEFAULT_NODE_ADDRESS);
    // Set the default transmit header values
    RF22_setHeaderTo(RF22_DEFAULT_NODE_ADDRESS);
    RF22_setHeaderFrom(RF22_DEFAULT_NODE_ADDRESS);
    RF22_setHeaderId(0);
    RF22_setHeaderFlags(0);

    // Ensure the antenna can be switched automatically according to transmit and receive
    // This assumes GPIO0(out) is connected to TX_ANT(in) to enable tx antenna during transmit
    // This assumes GPIO1(out) is connected to RX_ANT(in) to enable rx antenna during receive
    spiWrite (RF22_REG_0B_GPIO_CONFIGURATION0, 0x12) ; // TX state
    spiWrite (RF22_REG_0C_GPIO_CONFIGURATION1, 0x15) ; // RX state

    // Enable interrupts
    // this is original from arduion, which crashes on mbed after some hours
    //see https://groups.google.com/forum/?fromgroups#!topic/rf22-arduino/Ezkw256yQI8
    //spiWrite(RF22_REG_05_INTERRUPT_ENABLE1, RF22_ENTXFFAEM | RF22_ENRXFFAFULL | RF22_ENPKSENT | RF22_ENPKVALID | RF22_ENCRCERROR | RF22_ENFFERR);
    //without RF22_ENFFERR it works - Charly
    spiWrite(RF22_REG_05_INTERRUPT_ENABLE1, RF22_ENTXFFAEM |RF22_ENRXFFAFULL | RF22_ENPKSENT |RF22_ENPKVALID| RF22_ENCRCERROR);

    spiWrite(RF22_REG_06_INTERRUPT_ENABLE2, RF22_ENPREAVAL);


    // Set some defaults. An innocuous ISM frequency, and reasonable pull-in
    RF22_setFrequency(434.0, 0.05);
    RF22_setModemConfig(FSK_Rb125Fd125);
    // Minimum power
    RF22_setTxPower(RF22_TXPOW_8DBM);
    //setTxPower(RF22_TXPOW_17DBM);
    return true;
}

//------------------------------------------------------------------------
// C level interrupt handler for this instance
void RF22_handleInterrupt()
{
    uint8_t _lastInterruptFlags[2];

    // Read the interrupt flags which clears the interrupt
    spiBurstRead(RF22_REG_03_INTERRUPT_STATUS1, _lastInterruptFlags, 2);

    if (_lastInterruptFlags[0] & RF22_IFFERROR)
    {
    	RF22_resetFifos(); // Clears the interrupt
        if (_mode == RF22_MODE_TX)
        {
        	RF22_restartTransmit();
        }
        else if (_mode == RF22_MODE_RX)
        {
        	RF22_clearRxBuf();
            //stop and start Rx
        	RF22_setModeIdle();
        	RF22_setModeRx();
        }
        // stop handling the remaining interruppts as something went wrong here
        return;
    }
    
    // Caution, any delay here may cause a FF underflow or overflow
    if (_lastInterruptFlags[0] & RF22_ITXFFAEM)
    {
    	RF22_sendNextFragment();
    }
  
    if (_lastInterruptFlags[0] & RF22_IRXFFAFULL)
    {
    	RF22_readNextFragment();
    }   
    if (_lastInterruptFlags[0] & RF22_IEXT)
    {
    	RF22_handleExternalInterrupt();
    }
    if (_lastInterruptFlags[1] & RF22_IWUT)
    {

    	RF22_handleWakeupTimerInterrupt();
    }    
    if (_lastInterruptFlags[0] & RF22_IPKSENT)
    {
        _txGood++;
        _mode = RF22_MODE_IDLE;
    }
   
    if (_lastInterruptFlags[0] & RF22_IPKVALID)
    {
        uint8_t len = spiRead(RF22_REG_4B_RECEIVED_PACKET_LENGTH);

        // May have already read one or more fragments
        // Get any remaining unread octets, based on the expected length
        // First make sure we dont overflow the buffer in the case of a stupid length
        // or partial bad receives
        if (len >  RF22_MAX_MESSAGE_LEN
            || len < _bufLen)
        {
            _rxBad++;
            _mode = RF22_MODE_IDLE;
            RF22_clearRxBuf();
            return; // Hmmm receiver buffer overflow.
        }
        spiBurstRead(RF22_REG_7F_FIFO_ACCESS, _buf + _bufLen, len - _bufLen);
        //DISABLE_ALL_INTERRUPTS();    // Disable Interrupts
        _rxGood++;
        _bufLen = len;
        _mode = RF22_MODE_IDLE;
        _rxBufValid = true;
        // reset the fifo for next packet??
        //resetRxFifo();
        //ENABLE_ALL_INTERRUPTS();     // Enable Interrupts
    }
    
    if (_lastInterruptFlags[0] & RF22_ICRCERROR) {
        _rxBad++;
        RF22_clearRxBuf();
        RF22_resetRxFifo();
        _mode = RF22_MODE_IDLE;
        RF22_setModeRx(); // Keep trying
    }
    
    if (_lastInterruptFlags[1] & RF22_IPREAVAL) {      
        _lastRssi = spiRead(RF22_REG_26_RSSI);
        RF22_clearRxBuf();


    }
}

//------------------------------------------------------------------------
// These are low level functions that call the interrupt handler for the correct
// instance of RF22.
// 2 interrupts allows us to have 2 different devices
void RF22_isr0()
{
	RF22_handleInterrupt();
}

//------------------------------------------------------------------------
void RF22_reset()
{
	spiWrite(RF22_REG_07_OPERATING_MODE1, RF22_SWRES);
    // Wait for it to settle
	RF22_TimerWait_ms(1); // SWReset time is nominally 100usec
}

//------------------------------------------------------------------------
uint8_t RF22_statusRead()
{
    return spiRead(RF22_REG_02_DEVICE_STATUS);
}

//------------------------------------------------------------------------
uint8_t RF22_adcRead(uint8_t adcsel,
                      uint8_t adcref ,
                      uint8_t adcgain,
                      uint8_t adcoffs)
{
    uint8_t configuration = adcsel | adcref | (adcgain & RF22_ADCGAIN);
    spiWrite(RF22_REG_0F_ADC_CONFIGURATION, configuration | RF22_ADCSTART);
    spiWrite(RF22_REG_10_ADC_SENSOR_AMP_OFFSET, adcoffs);

    // Conversion time is nominally 305usec
    // Wait for the DONE bit
    while (!(spiRead(RF22_REG_0F_ADC_CONFIGURATION) & RF22_ADCDONE))
        ;
    // Return the value
    return spiRead(RF22_REG_11_ADC_VALUE);
}

//------------------------------------------------------------------------
uint8_t RF22_temperatureRead(uint8_t tsrange, uint8_t tvoffs)
{
    spiWrite(RF22_REG_12_TEMPERATURE_SENSOR_CALIBRATION, tsrange | RF22_ENTSOFFS);
    spiWrite(RF22_REG_13_TEMPERATURE_VALUE_OFFSET, tvoffs);
    return 1; //TODO RF22_adcRead(RF22_ADCSEL_INTERNAL_TEMPERATURE_SENSOR | RF22_ADCREF_BANDGAP_VOLTAGE);
}

//------------------------------------------------------------------------
uint16_t RF22_wutRead()
{
    uint8_t buf[2];
    spiBurstRead(RF22_REG_17_WAKEUP_TIMER_VALUE1, buf, 2);
    return ((uint16_t)buf[0] << 8) | buf[1]; // Dont rely on byte order
}

//------------------------------------------------------------------------
// RFM-22 doc appears to be wrong: WUT for wtm = 10000, r, = 0, d = 0 is about 1 sec
void RF22_setWutPeriod(uint16_t wtm, uint8_t wtr, uint8_t wtd)
{
    uint8_t period[3];

    period[0] = ((wtr & 0xf) << 2) | (wtd & 0x3);
    period[1] = wtm >> 8;
    period[2] = wtm & 0xff;
    spiBurstWrite(RF22_REG_14_WAKEUP_TIMER_PERIOD1, period, sizeof(period));
}

//------------------------------------------------------------------------
// Returns true if centre + (fhch * fhs) is within limits
// Caution, different versions of the RF22 support different max freq
// so YMMV
bool RF22_setFrequency(float centre, float afcPullInRange)
{
    uint8_t fbsel = RF22_SBSEL;
    uint8_t afclimiter;
    if (centre < 240.0 || centre > 960.0) // 930.0 for early silicon
        return false;
    if (centre >= 480.0) {
        if (afcPullInRange < 0.0 || afcPullInRange > 0.318750)
            return false;
        centre /= 2;
        fbsel |= RF22_HBSEL;
        afclimiter = afcPullInRange * 1000000.0 / 1250.0;
    } else {
        if (afcPullInRange < 0.0 || afcPullInRange > 0.159375)
            return false;
        afclimiter = afcPullInRange * 1000000.0 / 625.0;
    }
    centre /= 10.0;
    float integerPart = floor(centre);
    float fractionalPart = centre - integerPart;

    uint8_t fb = (uint8_t)integerPart - 24; // Range 0 to 23
    fbsel |= fb;
    uint16_t fc = fractionalPart * 64000;
    spiWrite(RF22_REG_73_FREQUENCY_OFFSET1, 0);  // REVISIT
    spiWrite(RF22_REG_74_FREQUENCY_OFFSET2, 0);
    spiWrite(RF22_REG_75_FREQUENCY_BAND_SELECT, fbsel);
    spiWrite(RF22_REG_76_NOMINAL_CARRIER_FREQUENCY1, fc >> 8);
    spiWrite(RF22_REG_77_NOMINAL_CARRIER_FREQUENCY0, fc & 0xff);
    spiWrite(RF22_REG_2A_AFC_LIMITER, afclimiter);
    return !(RF22_statusRead() & RF22_FREQERR);
}

//------------------------------------------------------------------------
// Step size in 10kHz increments
// Returns true if centre + (fhch * fhs) is within limits
bool RF22_setFHStepSize(uint8_t fhs)
{
    spiWrite(RF22_REG_7A_FREQUENCY_HOPPING_STEP_SIZE, fhs);
    return !(RF22_statusRead() & RF22_FREQERR);
}

//------------------------------------------------------------------------
// Adds fhch * fhs to centre frequency
// Returns true if centre + (fhch * fhs) is within limits
bool RF22_setFHChannel(uint8_t fhch)
{
    spiWrite(RF22_REG_79_FREQUENCY_HOPPING_CHANNEL_SELECT, fhch);
    return !(RF22_statusRead() & RF22_FREQERR);
}

//------------------------------------------------------------------------
uint8_t RF22_rssiRead()
{
    return spiRead(RF22_REG_26_RSSI);
}

//------------------------------------------------------------------------
uint8_t RF22_ezmacStatusRead()
{
    return spiRead(RF22_REG_31_EZMAC_STATUS);
}

//------------------------------------------------------------------------
void RF22_setMode(uint8_t mode)
{
    spiWrite(RF22_REG_07_OPERATING_MODE1, mode);
}

//------------------------------------------------------------------------
void RF22_setModeIdle()
{
    if (_mode != RF22_MODE_IDLE) {
    	RF22_setMode(_idleMode);
        _mode = RF22_MODE_IDLE;
    }
}

//------------------------------------------------------------------------
void RF22_setModeRx()
{
    if (_mode != RF22_MODE_RX) {
    	RF22_setMode(_idleMode | RF22_RXON);
        _mode = RF22_MODE_RX;
    }
}

//------------------------------------------------------------------------
void RF22_setModeTx()
{
    if (_mode != RF22_MODE_TX) {
    	RF22_setMode(_idleMode | RF22_TXON);
        _mode = RF22_MODE_TX;
        // Hmmm, if you dont clear the RX FIFO here, then it appears that going
        // to transmit mode in the middle of a receive can corrupt the
        // RX FIFO
        RF22_resetRxFifo();
//        clearRxBuf();
    }
}

//------------------------------------------------------------------------
uint8_t  RF22_mode()
{
    return _mode;
}

//------------------------------------------------------------------------
void RF22_setTxPower(uint8_t power)
{
    spiWrite(RF22_REG_6D_TX_POWER, power);
}

//------------------------------------------------------------------------
// Sets registers from a canned modem configuration structure
void RF22_setModemRegisters(const RF22_ModemConfig* config)
{
    spiWrite(RF22_REG_1C_IF_FILTER_BANDWIDTH,                    config->reg_1c);
    spiWrite(RF22_REG_1F_CLOCK_RECOVERY_GEARSHIFT_OVERRIDE,      config->reg_1f);
    spiBurstWrite(RF22_REG_20_CLOCK_RECOVERY_OVERSAMPLING_RATE, &config->reg_20, 6);
    spiBurstWrite(RF22_REG_2C_OOK_COUNTER_VALUE_1,              &config->reg_2c, 3);
    spiWrite(RF22_REG_58_CHARGE_PUMP_CURRENT_TRIMMING,           config->reg_58);
    spiWrite(RF22_REG_69_AGC_OVERRIDE1,                          config->reg_69);
    spiBurstWrite(RF22_REG_6E_TX_DATA_RATE1,                    &config->reg_6e, 5);
}

//------------------------------------------------------------------------
// Set one of the canned FSK Modem configs
// Returns true if its a valid choice
bool RF22_setModemConfig(RF22_ModemConfigChoice index)
{
    if (index > (sizeof(MODEM_CONFIG_TABLE) / sizeof(RF22_ModemConfig)))
        return false;

    RF22_ModemConfig cfg;
    memcpy(&cfg, &MODEM_CONFIG_TABLE[index], sizeof(RF22_ModemConfig));
    RF22_setModemRegisters(&cfg);

    return true;
}

//------------------------------------------------------------------------
// REVISIT: top bit is in Header Control 2 0x33
void RF22_setPreambleLength(uint8_t nibbles)
{
    spiWrite(RF22_REG_34_PREAMBLE_LENGTH, nibbles);
}

//------------------------------------------------------------------------
// Caution doesnt set sync word len in Header Control 2 0x33
void RF22_setSyncWords(const uint8_t* syncWords, uint8_t len)
{
    spiBurstWrite(RF22_REG_36_SYNC_WORD3, syncWords, len);
}

//------------------------------------------------------------------------
void RF22_clearRxBuf()
{
    DISABLE_ALL_INTERRUPTS();    // Disable Interrupts
    _bufLen = 0;
    _rxBufValid = false;
    ENABLE_ALL_INTERRUPTS();     // Enable Interrupts
}

//------------------------------------------------------------------------
bool RF22_available()
{
    if (!_rxBufValid)
    {
    	RF22_setModeRx(); // Make sure we are receiving
    }
    return _rxBufValid;
}

//------------------------------------------------------------------------
// Blocks until a valid message is received
void RF22_waitAvailable()
{
    while (!RF22_available());
}

//------------------------------------------------------------------------
// Blocks until a valid message is received or timeout expires
// Return true if there is a message available
bool RF22_waitAvailableTimeout(uint16_t timeout)
{
    uint32_t endtime = _time_counter + timeout;
    while (_time_counter < endtime)
    {
        if (RF22_available())
        {
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------
bool RF22_waitPacketSent(uint16_t timeout)
{   
    uint32_t endtime = _time_counter + timeout;
    while (_time_counter < endtime)
    {
        if(_mode != RF22_MODE_TX)
        {
            return true;
        }
    }
    return false;
}


//------------------------------------------------------------------------
bool RF22_recv(uint8_t* buf, uint8_t* len)
{
    if (!RF22_available())
    {
        return false;
    }
    DISABLE_ALL_INTERRUPTS();
    if (*len > _bufLen)
    {
        *len = _bufLen;
    }
    memcpy(buf, _buf, *len);
    RF22_clearRxBuf();
    ENABLE_ALL_INTERRUPTS();     // Enable Interrupts

    return true;
}

//------------------------------------------------------------------------
void RF22_clearTxBuf()
{
    DISABLE_ALL_INTERRUPTS();
    _bufLen = 0;
    _txBufSentIndex = 0;
    _txPacketSent = false;
    ENABLE_ALL_INTERRUPTS();     // Enable Interrupts
}

//------------------------------------------------------------------------
void RF22_startTransmit()
{
	RF22_sendNextFragment(); // Actually the first fragment
    spiWrite(RF22_REG_3E_PACKET_LENGTH, _bufLen); // Total length that will be sent
    RF22_setModeTx(); // Start the transmitter, turns off the receiver
}

//------------------------------------------------------------------------
// Restart the transmission of a packet that had a problem
void RF22_restartTransmit()
{
    _mode = RF22_MODE_IDLE;
    _txBufSentIndex = 0;
    RF22_startTransmit();
}

//------------------------------------------------------------------------
bool RF22_send(const uint8_t* data, uint8_t len)
{
	RF22_waitPacketSent(0);
    {
        if (!RF22_fillTxBuf(data, len))
        {
            return false;
        }
        RF22_startTransmit();
    }
    return true;
}

//------------------------------------------------------------------------
bool RF22_fillTxBuf(const uint8_t* data, uint8_t len)
{
	RF22_clearTxBuf();
    if (!len)
        return false;
    return RF22_appendTxBuf(data, len);
}

//------------------------------------------------------------------------
bool RF22_appendTxBuf(const uint8_t* data, uint8_t len)
{
    if (((uint16_t)_bufLen + len) > RF22_MAX_MESSAGE_LEN)
        return false;
    DISABLE_ALL_INTERRUPTS();    // Disable Interrupts
    memcpy(_buf + _bufLen, data, len);
    _bufLen += len;
    ENABLE_ALL_INTERRUPTS();     // Enable Interrupts
    return true;
}

//------------------------------------------------------------------------
// Assumption: there is currently <= RF22_TXFFAEM_THRESHOLD bytes in the Tx FIFO
void RF22_sendNextFragment()
{
    if (_txBufSentIndex < _bufLen) {
        // Some left to send?
        uint8_t len = _bufLen - _txBufSentIndex;
        // But dont send too much
        if (len > (RF22_FIFO_SIZE - RF22_TXFFAEM_THRESHOLD - 1))
        {
            len = (RF22_FIFO_SIZE - RF22_TXFFAEM_THRESHOLD - 1);
        }
        spiBurstWrite(RF22_REG_7F_FIFO_ACCESS, _buf + _txBufSentIndex, len);
        _txBufSentIndex += len;
    }
}

//------------------------------------------------------------------------
// Assumption: there are at least RF22_RXFFAFULL_THRESHOLD in the RX FIFO
// That means it should only be called after a RXFFAFULL interrupt
void RF22_readNextFragment()
{
    if (((uint16_t)_bufLen + RF22_RXFFAFULL_THRESHOLD) > RF22_MAX_MESSAGE_LEN)
        return; // Hmmm receiver overflow. Should never occur

    // Read the RF22_RXFFAFULL_THRESHOLD octets that should be there
    spiBurstRead(RF22_REG_7F_FIFO_ACCESS, _buf + _bufLen, RF22_RXFFAFULL_THRESHOLD);
    _bufLen += RF22_RXFFAFULL_THRESHOLD;
}

//------------------------------------------------------------------------
// Clear the FIFOs
void RF22_resetFifos()
{
    spiWrite(RF22_REG_08_OPERATING_MODE2, RF22_FFCLRRX | RF22_FFCLRTX);
    spiWrite(RF22_REG_08_OPERATING_MODE2, 0);
}

//------------------------------------------------------------------------
// Clear the Rx FIFO
void RF22_resetRxFifo()
{
    spiWrite(RF22_REG_08_OPERATING_MODE2, RF22_FFCLRRX);
    spiWrite(RF22_REG_08_OPERATING_MODE2, 0);
}

//------------------------------------------------------------------------
// CLear the TX FIFO
void RF22_resetTxFifo()
{
    spiWrite(RF22_REG_08_OPERATING_MODE2, RF22_FFCLRTX);
    spiWrite(RF22_REG_08_OPERATING_MODE2, 0);
}

//------------------------------------------------------------------------
// Default implmentation does nothing. Override if you wish
void RF22_handleExternalInterrupt()
{
}

//------------------------------------------------------------------------
// Default implmentation does nothing. Override if you wish
void RF22_handleWakeupTimerInterrupt()
{
}

//------------------------------------------------------------------------
void RF22_setHeaderTo(uint8_t to)
{
    spiWrite(RF22_REG_3A_TRANSMIT_HEADER3, to);
}

//------------------------------------------------------------------------
void RF22_setHeaderFrom(uint8_t from)
{
    spiWrite(RF22_REG_3B_TRANSMIT_HEADER2, from);
}

//------------------------------------------------------------------------
void RF22_setHeaderId(uint8_t id)
{
    spiWrite(RF22_REG_3C_TRANSMIT_HEADER1, id);
}

//------------------------------------------------------------------------
void RF22_setHeaderFlags(uint8_t flags)
{
    spiWrite(RF22_REG_3D_TRANSMIT_HEADER0, flags);
}

//------------------------------------------------------------------------
uint8_t RF22_headerTo()
{
    return spiRead(RF22_REG_47_RECEIVED_HEADER3);
}

//------------------------------------------------------------------------
uint8_t RF22_headerFrom()
{
    return spiRead(RF22_REG_48_RECEIVED_HEADER2);
}

//------------------------------------------------------------------------
uint8_t RF22_headerId()
{
    return spiRead(RF22_REG_49_RECEIVED_HEADER1);
}

//------------------------------------------------------------------------
uint8_t RF22_headerFlags()
{
    return spiRead(RF22_REG_4A_RECEIVED_HEADER0);
}

//------------------------------------------------------------------------
uint8_t RF22_lastRssi()
{
    return _lastRssi;
}

//------------------------------------------------------------------------
void RF22_setPromiscuous(bool promiscuous)
{
    spiWrite(RF22_REG_43_HEADER_ENABLE3, promiscuous ? 0x00 : 0xff);
}


//RFM23B Datagram
//------------------------------------------------------------------------
bool RF22_DatagramInit(uint8_t thisAddress)
{
    bool ret = RF22_init();
    if (ret)
    	RF22_setThisAddress(_thisAddress);
    return ret;
}

//------------------------------------------------------------------------
void RF22_setThisAddress(uint8_t thisAddress)
{
    _thisAddress = thisAddress;
    // Check the TO header against RF22_DEFAULT_NODE_ADDRESS
    spiWrite(RF22_REG_3F_CHECK_HEADER3, _thisAddress);
    // Use this address in the transmitted FROM header
    RF22_setHeaderFrom(_thisAddress);
}

//------------------------------------------------------------------------
bool RF22_sendto(uint8_t* buf, uint8_t len, uint8_t address)
{
	RF22_setHeaderTo(address);
    return RF22_send(buf, len);
}

//------------------------------------------------------------------------
bool RF22_recvfrom(uint8_t* buf, uint8_t* len, uint8_t* from, uint8_t* to, uint8_t* id, uint8_t* flags)
{
    if (RF22_recv(buf, len))
    {

    if (from)  *from =  RF22_headerFrom();
    if (to)    *to =    RF22_headerTo();
    if (id)    *id =    RF22_headerId();
    if (flags) *flags = RF22_headerFlags();
    return true;
    }
    return false;
}

////////////////////////////////////////////////////////////////////
// Public methods

//------------------------------------------------------------------------
void RF22_setTimeout(uint16_t timeout)
{
    _timeout = timeout;
}

//------------------------------------------------------------------------
void RF22_setRetries(uint8_t retries)
{
    _retries = retries;
}

//------------------------------------------------------------------------
bool RF22_sendtoWait(uint8_t* buf, uint8_t len, uint8_t address)
{
    // Assemble the message
    uint8_t thisSequenceNumber = ++_lastSequenceNumber;

    uint8_t retries = 0;
    while (retries++ <= _retries)
    {
    	RF22_setHeaderId(thisSequenceNumber);
    	RF22_setHeaderFlags(0);
    	RF22_sendto(buf, len, address);
    	RF22_waitPacketSent(3000);

        // Never wait for ACKS to broadcasts:
        if (address == RF22_BROADCAST_ADDRESS)
        {
            return true;
        }

        if (retries > 1)
        {
            _retransmissions++;
        }
        uint32_t thisSendTime = _time_counter; // Timeout does not include original transmit time

        // Compute a new timeout, random between _timeout and _timeout*2
        // This is to prevent collisions on every retransmit
        // if 2 nodes try to transmit at the same time
        uint16_t timeout = _timeout + (_timeout * (rand() % 100) / 100);
        while (_time_counter < (thisSendTime + timeout))
        {
            if (RF22_available())
            {
            	RF22_clearRxBuf(); // Not using recv, so clear it ourselves
                uint8_t from = RF22_headerFrom();
                uint8_t to = RF22_headerTo();
                uint8_t id = RF22_headerId();
                uint8_t flags = RF22_headerFlags();
                // Now have a message: is it our ACK?
                if (from == address
					&& to == _thisAddress
					&& (flags & RF22_FLAGS_ACK)
					&& (id == thisSequenceNumber))
				{
					// Its the ACK we are waiting for
					return true;
				}
                else if (!(flags & RF22_FLAGS_ACK)
                		 && (id == _seenIds[from]))
                {
                    // This is a request we have already received. ACK it again
        			RF22_acknowledge(id, from);
                }
                // Else discard it
            }
            // Not the one we are waiting for, maybe keep waiting until timeout exhausted
        }
        // Timeout exhausted, maybe retry
    }
    return false;
}

//------------------------------------------------------------------------
bool RF22_recvfromAck(uint8_t* buf, uint8_t* len, uint8_t* from, uint8_t* to, uint8_t* id, uint8_t* flags)
{
    uint8_t _from;
    uint8_t _to;
    uint8_t _id;
    uint8_t _flags;
    // Get the message before its clobbered by the ACK (shared rx anfd tx buffer in RF22
    if (RF22_available() && RF22_recvfrom(buf, len, &_from, &_to, &_id, &_flags))
    {
        // Never ACK an ACK
    if (!(_flags & RF22_FLAGS_ACK))
    {
            // Its a normal message for this node, not an ACK
        if (_to != RF22_BROADCAST_ADDRESS)
        {
                // Its not a broadcast, so ACK it
                // Acknowledge message with ACK set in flags and ID set to received ID
        		RF22_acknowledge(_id, _from);
            }
            // If we have not seen this message before, then we are interested in it
        if (_id != _seenIds[_from])
        {
                if (from)  *from =  _from;
                if (to)    *to =    _to;
                if (id)    *id =    _id;
                if (flags) *flags = _flags;
                _seenIds[_from] = _id;
                return true;
            }
            // Else just re-ack it and wait for a new one
        }
    }
    // No message for us available
    return false;
}

//------------------------------------------------------------------------
bool RF22_recvfromAckTimeout(uint8_t* buf, uint8_t* len, uint16_t timeout, uint8_t* from, uint8_t* to, uint8_t* id, uint8_t* flags)
{
    uint32_t endtime = _time_counter + timeout;
    while (_time_counter < endtime)
    {
        if (RF22_recvfromAck(buf, len, from, to, id, flags))
        {
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------
uint16_t RF22_retransmissions()
{
    return _retransmissions;
}

//------------------------------------------------------------------------
void RF22_acknowledge(uint8_t id, uint8_t from)
{
	RF22_setHeaderId(id);
	RF22_setHeaderFlags(RF22_FLAGS_ACK);
    // We would prefer to send a zero length ACK,
    // but if an RF22 receives a 0 length message with a CRC error, it will never receive
    // a 0 length message again, until its reset, which makes everything hang :-(
    // So we send an ACK of 1 octet
    // REVISIT: should we send the RSSI for the information of the sender?
    uint8_t ack = '!';
    RF22_sendto(&ack, sizeof(ack), from);
    RF22_waitPacketSent(10);
}
