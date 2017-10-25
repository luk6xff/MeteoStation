// RF22.h
// Author: Mike McCauley (mikem@open.com.au)
// Copyright (C) 2011 Mike McCauley
// $Id: RF22.h,v 1.23 2013/02/06 21:33:56 mikem Exp mikem $
//
// ported to mbed by Karl Zweimueller
// modified by Lukasz Uszko luszko@op.pl

/// \mainpage RF22 library for Arduino
///
/// This is the Arduino RF22 library.
/// It provides an object-oriented interface for sending and receiving data messages with Hope-RF
/// RF22B based radio modules, and compatible chips and modules, 
/// including the RFM22B transceiver module such as 
/// this bare module: http://www.sparkfun.com/products/10153
/// and this shield: https://www.sparkfun.com/products/11018
///
/// The Hope-RF (http://www.hoperf.com) RFM22B (http://www.hoperf.com/rf_fsk/fsk/RFM22B.htm) 
/// is a low-cost ISM transceiver module. It supports FSK, GFSK, OOK over a wide 
/// range of frequencies and programmable data rates.
///
/// This library provides functions for sending and receiving messages of up to 255 octets on any 
/// frequency supported by the RF22B, in a range of predefined data rates and frequency deviations. 
/// Frequency can be set with 312Hz precision to any frequency from 240.0MHz to 960.0MHz.
///
/// Up to 2 RF22B modules can be connected to an Arduino, permitting the construction of translators
/// and frequency changers, etc.
///
/// This library provides classes for 
/// - RF22: unaddressed, unreliable messages
/// - RF22Datagram: addressed, unreliable messages
/// - RF22ReliableDatagram: addressed, reliable, retransmitted, acknowledged messages.

/// The following modulation types are suppported with a range of modem configurations for 
/// common data rates and frequency deviations:
/// - GFSK Gaussian Frequency Shift Keying
/// - FSK Frequency Shift Keying
/// - OOK On-Off Keying
///
/// Support for other RF22B features such as on-chip temperature measurement, analog-digital 
/// converter, transmitter power control etc is also provided.
///
/// \par Packet Format
///
/// All messages sent and received by this RF22 library must conform to this packet format:
///
/// - 8 nibbles (4 octets) PREAMBLE
/// - 2 octets SYNC 0x2d, 0xd4
/// - 4 octets HEADER: (TO, FROM, ID, FLAGS)
/// - 1 octet LENGTH (0 to 255), number of octets in DATA
/// - 0 to 255 octets DATA
/// - 2 octets CRC computed with CRC16(IBM), computed on HEADER, LENGTH and DATA
///
/// For technical reasons, the message format is not compatible with the 
/// 'HopeRF Radio Transceiver Message Library for Arduino' http://www.open.com.au/mikem/arduino/HopeRF from the same author. Nor is it compatible with 
/// 'Virtual Wire' http://www.open.com.au/mikem/arduino/VirtualWire.pdf also from the same author.
///
/// \par Connecting RFM-22 to Arduino
///
/// If you have the Sparkfun RFM22 Shield (https://www.sparkfun.com/products/11018)
/// the connections described below are done for you on the shield, no changes required, 
/// just add headers and plug it in to an Arduino (but not and Arduino Mega, see below)
///
/// The physical connection between the RF22B and the Arduino require 3.3V, the 3 x SPI pins (SCK, SDI, SDO), 
/// a Slave Select pin and an interrupt pin.
/// Note also that on the RFF22B, it is required to control the TX_ANT and X_ANT pins of the RFM22 in order to enable the
/// antenna connection. The RF22 library is configured so that GPIO0 and GPIO1 outputs can control TX_ANT and RX_ANT input pins
/// automatically. You must connect GPIO0 to TX_ANT and GPIO1 to RX_ANT for this automatic antenna switching to occur.
///
/// Connect the RFM-22 to most Arduino's like this (Caution, Arduino Mega has different pins for SPI, 
/// see below):
/// \code
///                 Arduino      RFM-22B
///                 GND----------GND-\ (ground in)
///                              SDN-/ (shutdown in)
///                 3V3----------VCC   (3.3V in)
/// interrupt 0 pin D2-----------NIRQ  (interrupt request out)
///          SS pin D10----------NSEL  (chip select in)
///         SCK pin D13----------SCK   (SPI clock in)
///        MOSI pin D11----------SDI   (SPI Data in)
///        MISO pin D12----------SDO   (SPI data out)
///                           /--GPIO0 (GPIO0 out to control transmitter antenna TX_ANT
///                           \--TX_ANT (TX antenna control in)
///                           /--GPIO1 (GPIO1 out to control receiver antenna RX_ANT
///                           \--RX_ANT (RX antenna control in)
/// \endcode
/// For an Arduino Mega:
/// \code
///                 Mega         RFM-22B
///                 GND----------GND-\ (ground in)
///                              SDN-/ (shutdown in)
///                 3V3----------VCC   (3.3V in)
/// interrupt 0 pin D2-----------NIRQ  (interrupt request out)
///          SS pin D53----------NSEL  (chip select in)
///         SCK pin D52----------SCK   (SPI clock in)
///        MOSI pin D51----------SDI   (SPI Data in)
///        MISO pin D50----------SDO   (SPI data out)
///                           /--GPIO0 (GPIO0 out to control transmitter antenna TX_ANT
///                           \--TX_ANT (TX antenna control in)
///                           /--GPIO1 (GPIO1 out to control receiver antenna RX_ANT
///                           \--RX_ANT (RX antenna control in)
/// \endcode
/// and you can then use the default constructor RF22(). 
/// You can override the default settings for the SS pin and the interrupt 
/// in the RF22 constructor if you wish to connect the slave select SS to other than the normal one for your 
/// Arduino (D10 for Diecimila, Uno etc and D53 for Mega)
/// or the interrupt request to other than pin D2.
///
/// \par Interrupts
///
/// The RF22 library uses interrupts to react to events in the RF22 module, 
/// such as the reception of a new packet, or the completion of transmission of a packet. 
/// The RF22 library interrupt service routine reads status from and writes data
/// to the the RF22 module via the SPI interface. It is very important therefore,
/// that if you are using the RF22 library with another SPI based deviced, that you
/// disable interrupts while you transfer data to and from that other device.
/// Use cli() to disable interrupts and sei() to reenable them.
///
///
/// \par Automatic Frequency Control (AFC)
///
/// The RF22M modules use an inexpensive crystal to control the frequency synthesizer, and therfore you can expect 
/// the transmitter and receiver frequencies to be subject to the usual inaccuracies of such crystals. The RF22
/// contains an AFC circuit to compensate for differences in transmitter and receiver frequencies. 
/// It does this by altering the receiver frequency during reception by up to the pull-in frequency range. 
/// This RF22 library enables the AFC and by default sets the pull-in frequency range to
/// 0.05MHz, which should be sufficient to handle most situations. However, if you observe unexplained packet losses
/// or failure to operate correctly all the time it may be because your modules have a wider frequency difference, and
/// you may need to set the afcPullInRange to a differentvalue, using setFrequency();
///
/// Transmission (no reply) tests with modulation RF22::GFSK_Rb125Fd125 and a 
/// 13 octet message show about 330 messages per second transmitted.
///
/// Transmit-and-wait-for-a-reply tests with modulation RF22::GFSK_Rb125Fd125 and a 
/// 13 octet message (send and receive) show about 160 round trips per second.

/// This software is Copyright (C) 2011 Mike McCauley. Use is subject to license
/// conditions. The main licensing options available are GPL V2 or Commercial:
/// 
/// \par Open Source Licensing GPL V2
///
/// This is the appropriate option if you want to share the source code of your
/// application with everyone you distribute it to, and you also want to give them
/// the right to share who uses it. If you wish to use this software under Open
/// Source Licensing, you must contribute all your source code to the open source
/// community in accordance with the GPL Version 2 when your application is
/// distributed. See http://www.gnu.org/copyleft/gpl.html
/// \author  Mike McCauley (mikem@open.com.au)

#ifndef RF22_h
#define RF22_h

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
#include <stdint.h>
#include <stdbool.h>

/*******************************************************************************
* Defines
*******************************************************************************/
// This is the bit in the SPI address that marks it as a write
#define RF22_SPI_WRITE_MASK 0x80

// This is the maximum message length that can be supported by this library. Limited by
// the single message length octet in the header. 
// Yes, 255 is correct even though the FIFO size in the RF22 is only
// 64 octets. We use interrupts to refill the Tx FIFO during transmission and to empty the
// Rx FIFO during reception
// Can be pre-defined to a smaller size (to save SRAM) prior to including this header
#ifndef RF22_MAX_MESSAGE_LEN
#define RF22_MAX_MESSAGE_LEN 255
//#define RF22_MAX_MESSAGE_LEN 50
#endif

// Max number of octets the RF22 Rx and Tx FIFOs can hold
#define RF22_FIFO_SIZE 64

// Keep track of the mode the RF22 is in
#define RF22_MODE_IDLE         0
#define RF22_MODE_RX           1
#define RF22_MODE_TX           2

// These values we set for FIFO thresholds are actually the same as the POR values
#define RF22_TXFFAEM_THRESHOLD 4
#define RF22_RXFFAFULL_THRESHOLD 55

// This is the default node address,
#define RF22_DEFAULT_NODE_ADDRESS 0

// This address in the TO addreess signifies a broadcast
#define RF22_BROADCAST_ADDRESS 0xff

// Number of registers to be passed to setModemConfig()
#define RF22_NUM_MODEM_CONFIG_REGS 18

// Register names
#define RF22_REG_00_DEVICE_TYPE                         0x00
#define RF22_REG_01_VERSION_CODE                        0x01
#define RF22_REG_02_DEVICE_STATUS                       0x02
#define RF22_REG_03_INTERRUPT_STATUS1                   0x03
#define RF22_REG_04_INTERRUPT_STATUS2                   0x04
#define RF22_REG_05_INTERRUPT_ENABLE1                   0x05
#define RF22_REG_06_INTERRUPT_ENABLE2                   0x06
#define RF22_REG_07_OPERATING_MODE1                     0x07
#define RF22_REG_08_OPERATING_MODE2                     0x08
#define RF22_REG_09_OSCILLATOR_LOAD_CAPACITANCE         0x09
#define RF22_REG_0A_UC_OUTPUT_CLOCK                     0x0a
#define RF22_REG_0B_GPIO_CONFIGURATION0                 0x0b
#define RF22_REG_0C_GPIO_CONFIGURATION1                 0x0c
#define RF22_REG_0D_GPIO_CONFIGURATION2                 0x0d
#define RF22_REG_0E_IO_PORT_CONFIGURATION               0x0e
#define RF22_REG_0F_ADC_CONFIGURATION                   0x0f
#define RF22_REG_10_ADC_SENSOR_AMP_OFFSET               0x10
#define RF22_REG_11_ADC_VALUE                           0x11
#define RF22_REG_12_TEMPERATURE_SENSOR_CALIBRATION      0x12
#define RF22_REG_13_TEMPERATURE_VALUE_OFFSET            0x13
#define RF22_REG_14_WAKEUP_TIMER_PERIOD1                0x14
#define RF22_REG_15_WAKEUP_TIMER_PERIOD2                0x15
#define RF22_REG_16_WAKEUP_TIMER_PERIOD3                0x16
#define RF22_REG_17_WAKEUP_TIMER_VALUE1                 0x17
#define RF22_REG_18_WAKEUP_TIMER_VALUE2                 0x18
#define RF22_REG_19_LDC_MODE_DURATION                   0x19
#define RF22_REG_1A_LOW_BATTERY_DETECTOR_THRESHOLD      0x1a
#define RF22_REG_1B_BATTERY_VOLTAGE_LEVEL               0x1b
#define RF22_REG_1C_IF_FILTER_BANDWIDTH                 0x1c
#define RF22_REG_1D_AFC_LOOP_GEARSHIFT_OVERRIDE         0x1d
#define RF22_REG_1E_AFC_TIMING_CONTROL                  0x1e
#define RF22_REG_1F_CLOCK_RECOVERY_GEARSHIFT_OVERRIDE   0x1f
#define RF22_REG_20_CLOCK_RECOVERY_OVERSAMPLING_RATE    0x20
#define RF22_REG_21_CLOCK_RECOVERY_OFFSET2              0x21
#define RF22_REG_22_CLOCK_RECOVERY_OFFSET1              0x22
#define RF22_REG_23_CLOCK_RECOVERY_OFFSET0              0x23
#define RF22_REG_24_CLOCK_RECOVERY_TIMING_LOOP_GAIN1    0x24
#define RF22_REG_25_CLOCK_RECOVERY_TIMING_LOOP_GAIN0    0x25
#define RF22_REG_26_RSSI                                0x26
#define RF22_REG_27_RSSI_THRESHOLD                      0x27
#define RF22_REG_28_ANTENNA_DIVERSITY1                  0x28
#define RF22_REG_29_ANTENNA_DIVERSITY2                  0x29
#define RF22_REG_2A_AFC_LIMITER                         0x2a
#define RF22_REG_2B_AFC_CORRECTION_READ                 0x2b
#define RF22_REG_2C_OOK_COUNTER_VALUE_1                 0x2c
#define RF22_REG_2D_OOK_COUNTER_VALUE_2                 0x2d
#define RF22_REG_2E_SLICER_PEAK_HOLD                    0x2e
#define RF22_REG_30_DATA_ACCESS_CONTROL                 0x30
#define RF22_REG_31_EZMAC_STATUS                        0x31
#define RF22_REG_32_HEADER_CONTROL1                     0x32
#define RF22_REG_33_HEADER_CONTROL2                     0x33
#define RF22_REG_34_PREAMBLE_LENGTH                     0x34
#define RF22_REG_35_PREAMBLE_DETECTION_CONTROL1         0x35
#define RF22_REG_36_SYNC_WORD3                          0x36
#define RF22_REG_37_SYNC_WORD2                          0x37
#define RF22_REG_38_SYNC_WORD1                          0x38
#define RF22_REG_39_SYNC_WORD0                          0x39
#define RF22_REG_3A_TRANSMIT_HEADER3                    0x3a
#define RF22_REG_3B_TRANSMIT_HEADER2                    0x3b
#define RF22_REG_3C_TRANSMIT_HEADER1                    0x3c
#define RF22_REG_3D_TRANSMIT_HEADER0                    0x3d
#define RF22_REG_3E_PACKET_LENGTH                       0x3e
#define RF22_REG_3F_CHECK_HEADER3                       0x3f
#define RF22_REG_40_CHECK_HEADER2                       0x40
#define RF22_REG_41_CHECK_HEADER1                       0x41
#define RF22_REG_42_CHECK_HEADER0                       0x42
#define RF22_REG_43_HEADER_ENABLE3                      0x43
#define RF22_REG_44_HEADER_ENABLE2                      0x44
#define RF22_REG_45_HEADER_ENABLE1                      0x45
#define RF22_REG_46_HEADER_ENABLE0                      0x46
#define RF22_REG_47_RECEIVED_HEADER3                    0x47
#define RF22_REG_48_RECEIVED_HEADER2                    0x48
#define RF22_REG_49_RECEIVED_HEADER1                    0x49
#define RF22_REG_4A_RECEIVED_HEADER0                    0x4a
#define RF22_REG_4B_RECEIVED_PACKET_LENGTH              0x4b
#define RF22_REG_50_ANALOG_TEST_BUS_SELECT              0x50
#define RF22_REG_51_DIGITAL_TEST_BUS_SELECT             0x51
#define RF22_REG_52_TX_RAMP_CONTROL                     0x52
#define RF22_REG_53_PLL_TUNE_TIME                       0x53
#define RF22_REG_55_CALIBRATION_CONTROL                 0x55
#define RF22_REG_56_MODEM_TEST                          0x56
#define RF22_REG_57_CHARGE_PUMP_TEST                    0x57
#define RF22_REG_58_CHARGE_PUMP_CURRENT_TRIMMING        0x58
#define RF22_REG_59_DIVIDER_CURRENT_TRIMMING            0x59
#define RF22_REG_5A_VCO_CURRENT_TRIMMING                0x5a
#define RF22_REG_5B_VCO_CALIBRATION                     0x5b
#define RF22_REG_5C_SYNTHESIZER_TEST                    0x5c
#define RF22_REG_5D_BLOCK_ENABLE_OVERRIDE1              0x5d
#define RF22_REG_5E_BLOCK_ENABLE_OVERRIDE2              0x5e
#define RF22_REG_5F_BLOCK_ENABLE_OVERRIDE3              0x5f
#define RF22_REG_60_CHANNEL_FILTER_COEFFICIENT_ADDRESS  0x60
#define RF22_REG_61_CHANNEL_FILTER_COEFFICIENT_VALUE    0x61
#define RF22_REG_62_CRYSTAL_OSCILLATOR_POR_CONTROL      0x62
#define RF22_REG_63_RC_OSCILLATOR_COARSE_CALIBRATION    0x63
#define RF22_REG_64_RC_OSCILLATOR_FINE_CALIBRATION      0x64
#define RF22_REG_65_LDO_CONTROL_OVERRIDE                0x65
#define RF22_REG_66_LDO_LEVEL_SETTINGS                  0x66
#define RF22_REG_67_DELTA_SIGMA_ADC_TUNING1             0x67
#define RF22_REG_68_DELTA_SIGMA_ADC_TUNING2             0x68
#define RF22_REG_69_AGC_OVERRIDE1                       0x69
#define RF22_REG_6A_AGC_OVERRIDE2                       0x6a
#define RF22_REG_6B_GFSK_FIR_FILTER_COEFFICIENT_ADDRESS 0x6b
#define RF22_REG_6C_GFSK_FIR_FILTER_COEFFICIENT_VALUE   0x6c
#define RF22_REG_6D_TX_POWER                            0x6d
#define RF22_REG_6E_TX_DATA_RATE1                       0x6e
#define RF22_REG_6F_TX_DATA_RATE0                       0x6f
#define RF22_REG_70_MODULATION_CONTROL1                 0x70
#define RF22_REG_71_MODULATION_CONTROL2                 0x71
#define RF22_REG_72_FREQUENCY_DEVIATION                 0x72
#define RF22_REG_73_FREQUENCY_OFFSET1                   0x73
#define RF22_REG_74_FREQUENCY_OFFSET2                   0x74
#define RF22_REG_75_FREQUENCY_BAND_SELECT               0x75
#define RF22_REG_76_NOMINAL_CARRIER_FREQUENCY1          0x76
#define RF22_REG_77_NOMINAL_CARRIER_FREQUENCY0          0x77
#define RF22_REG_79_FREQUENCY_HOPPING_CHANNEL_SELECT    0x79
#define RF22_REG_7A_FREQUENCY_HOPPING_STEP_SIZE         0x7a
#define RF22_REG_7C_TX_FIFO_CONTROL1                    0x7c
#define RF22_REG_7D_TX_FIFO_CONTROL2                    0x7d
#define RF22_REG_7E_RX_FIFO_CONTROL                     0x7e
#define RF22_REG_7F_FIFO_ACCESS                         0x7f

// These register masks etc are named wherever possible
// corresponding to the bit and field names in the RF-22 Manual
// RF22_REG_00_DEVICE_TYPE                      0x00
#define RF22_DEVICE_TYPE_RX_TRX                 0x08
#define RF22_DEVICE_TYPE_TX                     0x07

// RF22_REG_02_DEVICE_STATUS                    0x02
#define RF22_FFOVL                              0x80
#define RF22_FFUNFL                             0x40
#define RF22_RXFFEM                             0x20
#define RF22_HEADERR                            0x10
#define RF22_FREQERR                            0x08
#define RF22_LOCKDET                            0x04
#define RF22_CPS                                0x03
#define RF22_CPS_IDLE                           0x00
#define RF22_CPS_RX                             0x01
#define RF22_CPS_TX                             0x10

// RF22_REG_03_INTERRUPT_STATUS1                0x03
#define RF22_IFFERROR                           0x80
#define RF22_ITXFFAFULL                         0x40
#define RF22_ITXFFAEM                           0x20
#define RF22_IRXFFAFULL                         0x10
#define RF22_IEXT                               0x08
#define RF22_IPKSENT                            0x04
#define RF22_IPKVALID                           0x02
#define RF22_ICRCERROR                          0x01

// RF22_REG_04_INTERRUPT_STATUS2                0x04
#define RF22_ISWDET                             0x80
#define RF22_IPREAVAL                           0x40
#define RF22_IPREAINVAL                         0x20
#define RF22_IRSSI                              0x10
#define RF22_IWUT                               0x08
#define RF22_ILBD                               0x04
#define RF22_ICHIPRDY                           0x02
#define RF22_IPOR                               0x01

// RF22_REG_05_INTERRUPT_ENABLE1                0x05
#define RF22_ENFFERR                            0x80
#define RF22_ENTXFFAFULL                        0x40
#define RF22_ENTXFFAEM                          0x20
#define RF22_ENRXFFAFULL                        0x10
#define RF22_ENEXT                              0x08
#define RF22_ENPKSENT                           0x04
#define RF22_ENPKVALID                          0x02
#define RF22_ENCRCERROR                         0x01

// RF22_REG_06_INTERRUPT_ENABLE2                0x06
#define RF22_ENSWDET                            0x80
#define RF22_ENPREAVAL                          0x40
#define RF22_ENPREAINVAL                        0x20
#define RF22_ENRSSI                             0x10
#define RF22_ENWUT                              0x08
#define RF22_ENLBDI                             0x04
#define RF22_ENCHIPRDY                          0x02
#define RF22_ENPOR                              0x01

// RF22_REG_07_OPERATING_MODE                   0x07
#define RF22_SWRES                              0x80
#define RF22_ENLBD                              0x40
#define RF22_ENWT                               0x20
#define RF22_X32KSEL                            0x10
#define RF22_TXON                               0x08
#define RF22_RXON                               0x04
#define RF22_PLLON                              0x02
#define RF22_XTON                               0x01

// RF22_REG_08_OPERATING_MODE2                  0x08
#define RF22_ANTDIV                             0xc0
#define RF22_RXMPK                              0x10
#define RF22_AUTOTX                             0x08
#define RF22_ENLDM                              0x04
#define RF22_FFCLRRX                            0x02
#define RF22_FFCLRTX                            0x01

// RF22_REG_0F_ADC_CONFIGURATION                0x0f
#define RF22_ADCSTART                           0x80
#define RF22_ADCDONE                            0x80
#define RF22_ADCSEL                             0x70
#define RF22_ADCSEL_INTERNAL_TEMPERATURE_SENSOR 0x00
#define RF22_ADCSEL_GPIO0_SINGLE_ENDED          0x10
#define RF22_ADCSEL_GPIO1_SINGLE_ENDED          0x20
#define RF22_ADCSEL_GPIO2_SINGLE_ENDED          0x30
#define RF22_ADCSEL_GPIO0_GPIO1_DIFFERENTIAL    0x40
#define RF22_ADCSEL_GPIO1_GPIO2_DIFFERENTIAL    0x50
#define RF22_ADCSEL_GPIO0_GPIO2_DIFFERENTIAL    0x60
#define RF22_ADCSEL_GND                         0x70
#define RF22_ADCREF                             0x0c
#define RF22_ADCREF_BANDGAP_VOLTAGE             0x00
#define RF22_ADCREF_VDD_ON_3                    0x08
#define RF22_ADCREF_VDD_ON_2                    0x0c
#define RF22_ADCGAIN                            0x03

// RF22_REG_10_ADC_SENSOR_AMP_OFFSET            0x10
#define RF22_ADCOFFS                            0x0f

// RF22_REG_12_TEMPERATURE_SENSOR_CALIBRATION   0x12
#define RF22_TSRANGE                            0xc0
#define RF22_TSRANGE_M64_64C                    0x00
#define RF22_TSRANGE_M64_192C                   0x40
#define RF22_TSRANGE_0_128C                     0x80
#define RF22_TSRANGE_M40_216F                   0xc0
#define RF22_ENTSOFFS                           0x20
#define RF22_ENTSTRIM                           0x10
#define RF22_TSTRIM                             0x0f

// RF22_REG_14_WAKEUP_TIMER_PERIOD1             0x14
#define RF22_WTR                                0x3c
#define RF22_WTD                                0x03

//  RF22_REG_1D_AFC_LOOP_GEARSHIFT_OVERRIDE     0x1d
#define RF22_AFBCD                              0x80
#define RF22_ENAFC                              0x40
#define RF22_AFCGEARH                           0x38
#define RF22_AFCGEARL                           0x07

// RF22_REG_1E_AFC_TIMING_CONTROL               0x1e
#define RF22_SWAIT_TIMER                        0xc0
#define RF22_SHWAIT                             0x38
#define RF22_ANWAIT                             0x07

// RF22_REG_30_DATA_ACCESS_CONTROL              0x30
#define RF22_ENPACRX                            0x80
#define RF22_MSBFRST                            0x00
#define RF22_LSBFRST                            0x40
#define RF22_CRCHDRS                            0x00
#define RF22_CRCDONLY                           0x20
#define RF22_ENPACTX                            0x08
#define RF22_ENCRC                              0x04
#define RF22_CRC                                0x03
#define RF22_CRC_CCITT                          0x00
#define RF22_CRC_CRC_16_IBM                     0x01
#define RF22_CRC_IEC_16                         0x02
#define RF22_CRC_BIACHEVA                       0x03

// RF22_REG_32_HEADER_CONTROL1                  0x32
#define RF22_BCEN                               0xf0
#define RF22_BCEN_NONE                          0x00
#define RF22_BCEN_HEADER0                       0x10
#define RF22_BCEN_HEADER1                       0x20
#define RF22_BCEN_HEADER2                       0x40
#define RF22_BCEN_HEADER3                       0x80
#define RF22_HDCH                               0x0f
#define RF22_HDCH_NONE                          0x00
#define RF22_HDCH_HEADER0                       0x01
#define RF22_HDCH_HEADER1                       0x02
#define RF22_HDCH_HEADER2                       0x04
#define RF22_HDCH_HEADER3                       0x08

// RF22_REG_33_HEADER_CONTROL2                  0x33
#define RF22_HDLEN                              0x70
#define RF22_HDLEN_0                            0x00
#define RF22_HDLEN_1                            0x10
#define RF22_HDLEN_2                            0x20
#define RF22_HDLEN_3                            0x30
#define RF22_HDLEN_4                            0x40
#define RF22_VARPKLEN                           0x00
#define RF22_FIXPKLEN                           0x08
#define RF22_SYNCLEN                            0x06
#define RF22_SYNCLEN_1                          0x00
#define RF22_SYNCLEN_2                          0x02
#define RF22_SYNCLEN_3                          0x04
#define RF22_SYNCLEN_4                          0x06
#define RF22_PREALEN8                           0x01

// RF22_REG_6D_TX_POWER                         0x6d
#define RF22_TXPOW                              0x07
#define RF22_TXPOW_4X31                         0x08 // Not used in RFM22B
#define RF22_TXPOW_1DBM                         0x00
#define RF22_TXPOW_2DBM                         0x01
#define RF22_TXPOW_5DBM                         0x02
#define RF22_TXPOW_8DBM                         0x03
#define RF22_TXPOW_11DBM                        0x04
#define RF22_TXPOW_14DBM                        0x05
#define RF22_TXPOW_17DBM                        0x06
#define RF22_TXPOW_20DBM                        0x07
// IN RFM23B
#define RF22_TXPOW_LNA_SW                       0x08

// RF22_REG_71_MODULATION_CONTROL2              0x71
#define RF22_TRCLK                              0xc0
#define RF22_TRCLK_NONE                         0x00
#define RF22_TRCLK_GPIO                         0x40
#define RF22_TRCLK_SDO                          0x80
#define RF22_TRCLK_NIRQ                         0xc0
#define RF22_DTMOD                              0x30
#define RF22_DTMOD_DIRECT_GPIO                  0x00
#define RF22_DTMOD_DIRECT_SDI                   0x10
#define RF22_DTMOD_FIFO                         0x20
#define RF22_DTMOD_PN9                          0x30
#define RF22_ENINV                              0x08
#define RF22_FD8                                0x04
#define RF22_MODTYP                             0x30
#define RF22_MODTYP_UNMODULATED                 0x00
#define RF22_MODTYP_OOK                         0x01
#define RF22_MODTYP_FSK                         0x02
#define RF22_MODTYP_GFSK                        0x03

// RF22_REG_75_FREQUENCY_BAND_SELECT            0x75
#define RF22_SBSEL                              0x40
#define RF22_HBSEL                              0x20
#define RF22_FB                                 0x1f

/*******************************************************************************
* Local Types and Typedefs
*******************************************************************************/
/// \brief Defines register values for a set of modem configuration registers
///
/// Defines register values for a set of modem configuration registers
/// that can be passed to setModemConfig()
/// if none of the choices in ModemConfigChoice suit your need
/// setModemConfig() writes the register values to the appropriate RF22 registers
/// to set the desired modulation type, data rate and deviation/bandwidth.
/// Suitable values for these registers can be computed using the register calculator at
/// http://www.hoperf.com/upload/rf/RF22B%2023B%2031B%2042B%2043B%20Register%20Settings_RevB1-v5.xls
typedef struct
{
	uint8_t    reg_1c;   ///< Value for register RF22_REG_1C_IF_FILTER_BANDWIDTH
	uint8_t    reg_1f;   ///< Value for register RF22_REG_1F_CLOCK_RECOVERY_GEARSHIFT_OVERRIDE
	uint8_t    reg_20;   ///< Value for register RF22_REG_20_CLOCK_RECOVERY_OVERSAMPLING_RATE
	uint8_t    reg_21;   ///< Value for register RF22_REG_21_CLOCK_RECOVERY_OFFSET2
	uint8_t    reg_22;   ///< Value for register RF22_REG_22_CLOCK_RECOVERY_OFFSET1
	uint8_t    reg_23;   ///< Value for register RF22_REG_23_CLOCK_RECOVERY_OFFSET0
	uint8_t    reg_24;   ///< Value for register RF22_REG_24_CLOCK_RECOVERY_TIMING_LOOP_GAIN1
	uint8_t    reg_25;   ///< Value for register RF22_REG_25_CLOCK_RECOVERY_TIMING_LOOP_GAIN0
	uint8_t    reg_2c;   ///< Value for register RF22_REG_2C_OOK_COUNTER_VALUE_1
	uint8_t    reg_2d;   ///< Value for register RF22_REG_2D_OOK_COUNTER_VALUE_2
	uint8_t    reg_2e;   ///< Value for register RF22_REG_2E_SLICER_PEAK_HOLD
	uint8_t    reg_58;   ///< Value for register RF22_REG_58_CHARGE_PUMP_CURRENT_TRIMMING
	uint8_t    reg_69;   ///< Value for register RF22_REG_69_AGC_OVERRIDE1
	uint8_t    reg_6e;   ///< Value for register RF22_REG_6E_TX_DATA_RATE1
	uint8_t    reg_6f;   ///< Value for register RF22_REG_6F_TX_DATA_RATE0
	uint8_t    reg_70;   ///< Value for register RF22_REG_70_MODULATION_CONTROL1
	uint8_t    reg_71;   ///< Value for register RF22_REG_71_MODULATION_CONTROL2
	uint8_t    reg_72;   ///< Value for register RF22_REG_72_FREQUENCY_DEVIATION
} RF22_ModemConfig;

/// Choices for setModemConfig() for a selected subset of common modulation types,
/// and data rates. If you need another configuration, use the register calculator.
/// and call setModemRegisters() with your desired settings
/// These are indexes into _modemConfig
typedef enum
{
	UnmodulatedCarrier = 0, ///< Unmodulated carrier for testing
	FSK_PN9_Rb2Fd5,      ///< FSK, No Manchester, Rb = 2kbs, Fd = 5kHz, PN9 random modulation for testing

	FSK_Rb2Fd5,         ///< FSK, No Manchester, Rb = 2kbs,    Fd = 5kHz
	FSK_Rb2_4Fd36,       ///< FSK, No Manchester, Rb = 2.4kbs,  Fd = 36kHz
	FSK_Rb4_8Fd45,       ///< FSK, No Manchester, Rb = 4.8kbs,  Fd = 45kHz
	FSK_Rb9_6Fd45,       ///< FSK, No Manchester, Rb = 9.6kbs,  Fd = 45kHz
	FSK_Rb19_2Fd9_6,     ///< FSK, No Manchester, Rb = 19.2kbs, Fd = 9.6kHz
	FSK_Rb38_4Fd19_6,    ///< FSK, No Manchester, Rb = 38.4kbs, Fd = 19.6kHz
	FSK_Rb57_6Fd28_8,    ///< FSK, No Manchester, Rb = 57.6kbs, Fd = 28.8kHz
	FSK_Rb125Fd125,      ///< FSK, No Manchester, Rb = 125kbs,  Fd = 125kHz

	GFSK_Rb2Fd5,         ///< GFSK, No Manchester, Rb = 2kbs,    Fd = 5kHz
	GFSK_Rb2_4Fd36,      ///< GFSK, No Manchester, Rb = 2.4kbs,  Fd = 36kHz
	GFSK_Rb4_8Fd45,      ///< GFSK, No Manchester, Rb = 4.8kbs,  Fd = 45kHz
	GFSK_Rb9_6Fd45,      ///< GFSK, No Manchester, Rb = 9.6kbs,  Fd = 45kHz
	GFSK_Rb19_2Fd9_6,    ///< GFSK, No Manchester, Rb = 19.2kbs, Fd = 9.6kHz
	GFSK_Rb38_4Fd19_6,   ///< GFSK, No Manchester, Rb = 38.4kbs, Fd = 19.6kHz
	GFSK_Rb57_6Fd28_8,   ///< GFSK, No Manchester, Rb = 57.6kbs, Fd = 28.8kHz
	GFSK_Rb125Fd125,     ///< GFSK, No Manchester, Rb = 125kbs,  Fd = 125kHz

	OOK_Rb1_2Bw75,       ///< OOK, No Manchester, Rb = 1.2kbs,  Rx Bandwidth = 75kHz
	OOK_Rb2_4Bw335,      ///< OOK, No Manchester, Rb = 2.4kbs,  Rx Bandwidth = 335kHz
	OOK_Rb4_8Bw335,      ///< OOK, No Manchester, Rb = 4.8kbs,  Rx Bandwidth = 335kHz
	OOK_Rb9_6Bw335,      ///< OOK, No Manchester, Rb = 9.6kbs,  Rx Bandwidth = 335kHz
	OOK_Rb19_2Bw335,     ///< OOK, No Manchester, Rb = 19.2kbs, Rx Bandwidth = 335kHz
	OOK_Rb38_4Bw335,     ///< OOK, No Manchester, Rb = 38.4kbs, Rx Bandwidth = 335kHz
	OOK_Rb40Bw335        ///< OOK, No Manchester, Rb = 40kbs,   Rx Bandwidth = 335kHz
} RF22_ModemConfigChoice;

/*******************************************************************************
* Public function declarations
*******************************************************************************/
/// Initialises this instance and the radio module connected to it.
/// The following steps are taken:
/// - Initialise the slave select pin and the SPI interface library
/// - Software reset the RF22 module
/// - Checks the connected RF22 module is either a RF22_DEVICE_TYPE_RX_TRX or a RF22_DEVICE_TYPE_TX
/// - Attaches an interrupt handler
/// - Configures the RF22 module
/// - Sets the frequncy to 434.0 MHz
/// - Sets the modem data rate to FSK_Rb2_4Fd36
/// \return  true if everything was successful
bool RF22_init();

/// Issues a software reset to the
/// RF22 module. Blocks for 1ms to ensure the reset is complete.
void RF22_reset();

/// Reads and returns the device status register RF22_REG_02_DEVICE_STATUS
/// \return The value of the device status register
uint8_t RF22_statusRead();

/// Reads a value from the on-chip analog-digital converter
/// \param[in] adcsel Selects the ADC input to measure. One of RF22_ADCSEL_*. Defaults to the
/// internal temperature sensor
/// \param[in] adcref Specifies the refernce voltage to use. One of RF22_ADCREF_*.
/// Defaults to the internal bandgap voltage.
/// \param[in] adcgain Amplifier gain selection.
/// \param[in] adcoffs Amplifier offseet (0 to 15).
/// \return The analog value. 0 to 255.
uint8_t RF22_adcRead(uint8_t adcsel,
					 uint8_t adcref,
					 uint8_t adcgain,
					 uint8_t adcoffs);

/// Reads the on-chip temperature sensoer
/// \param[in] tsrange Specifies the temperature range to use. One of RF22_TSRANGE_*
/// \param[in] tvoffs Specifies the temperature value offset. This is actually signed value
/// added to the measured temperature value
/// \return The measured temperature.
uint8_t RF22_temperatureRead(uint8_t tsrange, uint8_t tvoffs);

/// Reads the wakeup timer value in registers RF22_REG_17_WAKEUP_TIMER_VALUE1
/// and RF22_REG_18_WAKEUP_TIMER_VALUE2
/// \return The wakeup timer value
uint16_t RF22_wutRead();

/// Sets the wakeup timer period registers RF22_REG_14_WAKEUP_TIMER_PERIOD1,
/// RF22_REG_15_WAKEUP_TIMER_PERIOD2 and RF22_R<EG_16_WAKEUP_TIMER_PERIOD3
/// \param[in] wtm Wakeup timer mantissa value
/// \param[in] wtr Wakeup timer exponent R value
/// \param[in] wtd Wakeup timer exponent D value
void RF22_setWutPeriod(uint16_t wtm, uint8_t wtr, uint8_t wtd);

/// Sets the transmitter and receiver centre frequency
/// \param[in] centre Frequency in MHz. 240.0 to 960.0. Caution, some versions of RF22 and derivatives
/// implemented more restricted frequency ranges.
/// \param[in] afcPullInRange Sets the AF Pull In Range in MHz. Defaults to 0.05MHz (50kHz). Range is 0.0 to 0.159375
/// for frequencies 240.0 to 480MHz, and 0.0 to 0.318750MHz for  frequencies 480.0 to 960MHz,
/// \return true if the selected frquency centre + (fhch * fhs) is within range and the afcPullInRange is within range
bool RF22_setFrequency(float centre, float afcPullInRange);

/// Sets the frequency hopping step size.
/// \param[in] fhs Frequency Hopping step size in 10kHz increments
/// \return true if centre + (fhch * fhs) is within limits
bool RF22_setFHStepSize(uint8_t fhs);

/// Sets the frequncy hopping channel. Adds fhch * fhs to centre frequency
/// \param[in] fhch The channel number
/// \return true if the selected frquency centre + (fhch * fhs) is within range
bool RF22_setFHChannel(uint8_t fhch);

/// Reads and returns the current RSSI value from register RF22_REG_26_RSSI. If you want to find the RSSI
/// of the last received message, use lastRssi() instead.
/// \return The current RSSI value
uint8_t RF22_rssiRead();

/// Reads and returns the current EZMAC value from register RF22_REG_31_EZMAC_STATUS
/// \return The current EZMAC value
uint8_t RF22_ezmacStatusRead();

/// Sets the parameters for the RF22 Idle mode in register RF22_REG_07_OPERATING_MODE.
/// Idle mode is the mode the RF22 will be in when not transmitting or receiving. The default idle mode
/// is RF22_XTON ie READY mode.
/// \param[in] mode Mask of mode bits, using RF22_SWRES, RF22_ENLBD, RF22_ENWT,
/// RF22_X32KSEL, RF22_PLLON, RF22_XTON.
void RF22_setMode(uint8_t mode);

/// If current mode is Rx or Tx changes it to Idle. If the transmitter or receiver is running,
/// disables them.
void RF22_setModeIdle();

/// If current mode is Tx or Idle, changes it to Rx.
/// Starts the receiver in the RF22.
void RF22_setModeRx();

/// If current mode is Rx or Idle, changes it to Rx.
/// Starts the transmitter in the RF22.
void RF22_setModeTx();

/// Returns the operating mode of the library.
/// \return the current mode, one of RF22_MODE_*
uint8_t RF22_mode();

/// Sets the transmitter power output level in register RF22_REG_6D_TX_POWER.
/// Be a good neighbour and set the lowest power level you need.
/// After init(), the power wil be set to RF22_TXPOW_8DBM.
/// Caution: In some countries you may only select RF22_TXPOW_17DBM if you
/// are also using frequency hopping.
/// \param[in] power Transmitter power level, one of RF22_TXPOW_*
void RF22_setTxPower(uint8_t power);

/// Sets all the registered required to configure the data modem in the RF22, including the data rate,
/// bandwidths etc. You cas use this to configure the modem with custom configuraitons if none of the
/// canned configurations in ModemConfigChoice suit you.
/// \param[in] config A ModemConfig structure containing values for the modem configuration registers.
void RF22_setModemRegisters(const RF22_ModemConfig* config);

/// Select one of the predefined modem configurations. If you need a modem configuration not provided
/// here, use setModemRegisters() with your own ModemConfig.
/// \param[in] index The configuration choice.
/// \return true if index is a valid choice.
bool RF22_setModemConfig(RF22_ModemConfigChoice index);

/// Starts the receiver and checks whether a received message is available.
/// This can be called multiple times in a timeout loop
/// \return true if a complete, valid message has been received and is able to be retrieved by
/// recv()
bool RF22_available();

/// Starts the receiver and blocks until a valid received
/// message is available.
void RF22_waitAvailable();

/// Starts the receiver and blocks until a received message is available or a timeout
/// \param[in] timeout Maximum time to wait in milliseconds.
/// \return true if a message is available
bool RF22_waitAvailableTimeout(uint16_t timeout);

/// Turns the receiver on if it not already on.
/// If there is a valid message available, copy it to buf and return true
/// else return false.
/// If a message is copied, *len is set to the length (Caution, 0 length messages are permitted).
/// You should be sure to call this function frequently enough to not miss any messages
/// It is recommended that you call it in your main loop.
/// \param[in] buf Location to copy the received message
/// \param[in,out] len Pointer to available space in buf. Set to the actual number of octets copied.
/// \return true if a valid message was copied to buf
bool RF22_recv(uint8_t* buf, uint8_t* len);

/// Waits until any previous transmit packet is finished being transmitted with waitPacketSent().
/// Then loads a message into the transmitter and starts the transmitter. Note that a message length
/// of 0 is NOT permitted.
/// \param[in] data Array of data to be sent
/// \param[in] len Number of bytes of data to send (> 0)
/// \return true if the message length was valid and it was correctly queued for transmit
bool RF22_send(const uint8_t* data, uint8_t len);

/// Blocks until the RF22 is not in mode RF22_MODE_TX (ie until the RF22 is not transmitting).
/// This effectively waits until any previous transmit packet is finished being transmitted.
bool RF22_waitPacketSent(uint16_t timeout);

/// Tells the receiver to accept messages with any TO address, not just messages
/// addressed to this node or the broadcast address
/// \param[in] promiscuous true if you wish to receive messages with any TO address
void RF22_setPromiscuous(bool promiscuous);

/// Returns the TO header of the last received message
/// \return The TO header
uint8_t RF22_headerTo();

/// Returns the FROM header of the last received message
/// \return The FROM header
uint8_t RF22_headerFrom();

/// Returns the ID header of the last received message
/// \return The ID header
uint8_t RF22_headerId();

/// Returns the FLAGS header of the last received message
/// \return The FLAGS header
uint8_t RF22_headerFlags();

/// Returns the RSSI (Receiver Signal Strength Indicator)
/// of the last received message. This measurement is taken when
/// the preamble has been received. It is a (non-linear) measure of the received signal strength.
/// \return The RSSI
uint8_t RF22_lastRssi();

/// Sets the length of the preamble
/// in 4-bit nibbles.
/// Caution: this should be set to the same
/// value on all nodes in your network. Default is 8.
/// Sets the message preamble length in RF22_REG_34_PREAMBLE_LENGTH
/// \param[in] nibbles Preamble length in nibbles of 4 bits each.
void RF22_setPreambleLength(uint8_t nibbles);

/// Sets the sync words for transmit and receive in registers RF22_REG_36_SYNC_WORD3
/// to RF22_REG_39_SYNC_WORD0
/// Caution: this should be set to the same
/// value on all nodes in your network. Default is { 0x2d, 0xd4 }
/// \param[in] syncWords Array of sync words
/// \param[in] len Number of sync words to set
void RF22_setSyncWords(const uint8_t* syncWords, uint8_t len);

/// This is a low level function to handle the interrupts for one instance of RF22.
/// Called automatically by isr0() and isr1()
/// Should not need to be called.
void RF22_handleInterrupt();

/// Clears the receiver buffer.
/// Internal use only
void RF22_clearRxBuf();

/// Clears the transmitter buffer
/// Internal use only
void RF22_clearTxBuf();

/// Fills the transmitter buffer with the data of a mesage to be sent
/// \param[in] data Array of data bytes to be sent (1 to 255)
/// \param[in] len Number of data bytes in data (> 0)
/// \return true if the message length is valid
bool RF22_fillTxBuf(const uint8_t* data, uint8_t len);

/// Appends the transmitter buffer with the data of a mesage to be sent
/// \param[in] data Array of data bytes to be sent (0 to 255)
/// \param[in] len Number of data bytes in data
/// \return false if the resulting message would exceed RF22_MAX_MESSAGE_LEN, else true
bool RF22_appendTxBuf(const uint8_t* data, uint8_t len);

/// Internal function to load the next fragment of
/// the current message into the transmitter FIFO
/// Internal use only
void RF22_sendNextFragment();

///  function to copy the next fragment from
/// the receiver FIF) into the receiver buffer
void RF22_readNextFragment();

/// Clears the RF22 Rx and Tx FIFOs
/// Internal use only
void RF22_resetFifos();

/// Clears the RF22 Rx FIFO
/// Internal use only
void RF22_resetRxFifo();

/// Clears the RF22 Tx FIFO
/// Internal use only
void RF22_resetTxFifo();

/// This function will be called by handleInterrupt() if an RF22 external interrupt occurs.
/// This can only happen if external interrupts are enabled in the RF22
/// (which they are not by default).
/// Subclasses may override this function to get control when  an RF22 external interrupt occurs.
void  RF22_handleExternalInterrupt();

/// This function will be called by handleInterrupt() if an RF22 wakeup timer interrupt occurs.
/// This can only happen if wakeup timer interrupts are enabled in the RF22
/// (which they are not by default).
/// Subclasses may override this function to get control when  an RF22 wakeup timer interrupt occurs.
void  RF22_handleWakeupTimerInterrupt();

/// Sets the TO header to be sent in all subsequent messages
/// \param[in] to The new TO header value
void RF22_setHeaderTo(uint8_t to);

/// Sets the FROM header to be sent in all subsequent messages
/// \param[in] from The new FROM header value
void RF22_setHeaderFrom(uint8_t from);

/// Sets the ID header to be sent in all subsequent messages
/// \param[in] id The new ID header value
void RF22_setHeaderId(uint8_t id);

/// Sets the FLAGS header to be sent in all subsequent messages
/// \param[in] flags The new FLAGS header value
void RF22_setHeaderFlags(uint8_t flags);

/// Start the transmission of the contents
/// of the Tx buffer
void RF22_startTransmit();

/// ReStart the transmission of the contents
/// of the Tx buffer after a atransmission failure
void RF22_restartTransmit();


/// Low level interrupt service routine for RF22 connected to interrupt 0
void RF22_isr0();

//DATAGRAM
/// Initialises this instance and the radio module connected to it.
/// Overrides the init() function in RF22
bool RF22_DatagramInit(uint8_t thisAddress);

/// Sets the address of this node. Defaults to 0.
/// This will be used to set the FROM address of all messages sent by this node.
/// If all the nodes leave the address unset (ie 0),
/// In a conventional multinode system, all nodes will have a unique address
/// (which you could store in EEPROM).
/// \param[in] thisAddress The address of this node
void RF22_setThisAddress(uint8_t thisAddress);

/// Sends a message to the node(s) with the given address
/// RF22_BROADCAST_ADDRESS is a valid address which will cause the message
/// to be accepted by all RF22Datagram nodes within range.
/// \param[in] buf Pointer to the binary message to send
/// \param[in] len Number of octets to send (> 0)
/// \param[in] address The address to send the message to.
/// \return true if the message was transmitted.
bool RF22_sendto(uint8_t* buf, uint8_t len, uint8_t address);

/// Turns the receiver on if it not already on.
/// If there is a valid message available for this node, copy it to buf and return true
/// The SRC address is placed in *from if present and not NULL.
/// The DEST address is placed in *to if present and not NULL.
/// If a message is copied, *len is set to the length.
/// You should be sure to call this function frequently enough to not miss any messages
/// It is recommended that you call it in your main loop.
/// \param[in] buf Location to copy the received message
/// \param[in,out] len Pointer to available space in buf. Set to the actual number of octets copied.
/// \param[in] from If present and not NULL, the referenced uint8_t will be set to the FROM address
/// \param[in] to If present and not NULL, the referenced uint8_t will be set to the TO address
/// \param[in] id If present and not NULL, the referenced uint8_t will be set to the ID
/// \param[in] flags If present and not NULL, the referenced uint8_t will be set to the FLAGS
/// (not just those addressed to this node).
/// \return true if a valid message was copied to buf
bool RF22_recvfrom(uint8_t* buf, uint8_t* len, uint8_t* from, uint8_t* to, uint8_t* id, uint8_t* flags);


/// Sets the minimum retransmit timeout. If sendtoWait is waiting for an ack
/// longer than this time (in milliseconds),
/// it will retransmit the message. Defaults to 200ms. The timeout is measured from the end of
/// transmission of the message. It must be at least longer than the the transmit
/// time of the acknowledgement (preamble+6 octets) plus the latency/poll time of the receiver.
/// For fast modulation schemes you can considerably shorten this time.
/// The actual timeout is randomly varied between timeout and timeout*2.
/// \param[in] timeout The new timeout period in milliseconds
void RF22_setTimeout(uint16_t timeout);

/// Sets the max number of retries. Defaults to 3. If set to 0, the message will only be sent once.
/// sendtoWait will give up and return false if there is no ack received after all transmissions time out.
/// param[in] retries The maximum number a retries.
void RF22_setRetries(uint8_t retries);

/// Send the message and waits for an ack. Returns true if an acknowledgement is received.
/// Synchronous: any message other than the desired ACK received while waiting is discarded.
/// Blocks until an ACK is received or all retries are exhausted (ie up to retries*timeout milliseconds).
/// \param[in] address The address to send the message to.
/// \param[in] buf Pointer to the binary message to send
/// \param[in] len Number of octets to send
/// \return true if the message was transmitted and an acknowledgement was received.
bool RF22_sendtoWait(uint8_t* buf, uint8_t len, uint8_t address);

/// If there is a valid message available for this node, send an acknowledgement to the SRC
/// address (blocking until this is complete), then copy the message to buf and return true
/// else return false.
/// If a message is copied, *len is set to the length..
/// If from is not NULL, the SRC address is placed in *from.
/// If to is not NULL, the DEST address is placed in *to.
/// This is the preferred function for getting messages addressed to this node.
/// If the message is not a broadcast, acknowledge to the sender before returning.
/// You should be sure to call this function frequently enough to not miss any messages
/// It is recommended that you call it in your main loop.
/// \param[in] buf Location to copy the received message
/// \param[in,out] len Available space in buf. Set to the actual number of octets copied.
/// \param[in] from If present and not NULL, the referenced uint8_t will be set to the SRC address
/// \param[in] to If present and not NULL, the referenced uint8_t will be set to the DEST address
/// \param[in] id If present and not NULL, the referenced uint8_t will be set to the ID
/// \param[in] flags If present and not NULL, the referenced uint8_t will be set to the FLAGS
/// (not just those addressed to this node).
/// \return true if a valid message was copied to buf
bool RF22_recvfromAck(uint8_t* buf, uint8_t* len, uint8_t* from, uint8_t* to, uint8_t* id, uint8_t* flags);

/// Similar to recvfromAck(), this will block until either a valid message available for this node
/// or the timeout expires. Starts the receiver automatically.
/// You should be sure to call this function frequently enough to not miss any messages
/// It is recommended that you call it in your main loop.
/// \param[in] buf Location to copy the received message
/// \param[in,out] len Available space in buf. Set to the actual number of octets copied.
/// \param[in] timeout Maximum time to wait in milliseconds
/// \param[in] from If present and not NULL, the referenced uint8_t will be set to the SRC address
/// \param[in] to If present and not NULL, the referenced uint8_t will be set to the DEST address
/// \param[in] id If present and not NULL, the referenced uint8_t will be set to the ID
/// \param[in] flags If present and not NULL, the referenced uint8_t will be set to the FLAGS
/// (not just those addressed to this node).
/// \return true if a valid message was copied to buf
bool RF22_recvfromAckTimeout(uint8_t* buf, uint8_t* len,  uint16_t timeout, uint8_t* from, uint8_t* to, uint8_t* id, uint8_t* flags);

/// Returns the number of retransmissions
/// we have had to send
/// \return The number of retransmissions since initialisation.
uint16_t RF22_retransmissions();

/// Send an ACK for the message id to the given from address
/// Blocks until the ACK has been sent
void RF22_acknowledge(uint8_t id, uint8_t from);

/// Checks whether the message currently in the Rx buffer is a new message, not previously received
/// based on the from address and the sequence.  If it is new, it is acknowledged and returns true
/// \return true if there is a message received and it is a new message
bool RF22_haveNewMessage();



#endif 
