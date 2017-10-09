/*
 * esp8266.h
 *
 *  Created on: 20 lis 2016
 *      Author: igbt6
 */

#ifndef ESP8266_H_
#define ESP8266_H_

#include <stdint.h>
#include <stdbool.h>



#define ESP8266_TX_BUF_SIZE 256
#define ESP8266_RX_BUF_SIZE 1024


typedef enum
{
	ESP8266_MODE_CLIENT = 1,
	ESP8266_MODE_AP,
	ESP8266_MODE_AP_AND_CLIENT,
	ESP8266_MODE_INVALID
}Esp8266WifiMode;


typedef enum
{
	ESP8266_PROTOCOL_TCP = 0,
	ESP8266_PROTOCOL_UDP = 1,
	ESP8266_PROTOCOL_NUM
}Esp8266Protocol;



void esp8266Init();

//
//@brief Blocks SW for some time and searches for a given number of bytes
//@param  num_of_bytes - number of bytes which must be received into RX buffer from the module
//@param  ms_timeout - timeout for which given data shall be found in
//@param  pattern - a some pattern which we expect to find the RX buffer, if not needed pass by NULL
//@return false-given number of bytes or/and pattern found, true-success
//
bool esp8266WaitForData(uint16_t num_of_bytes, uint16_t ms_timeout, uint16_t ms_time_for_more_data, const char* pattern);


//
//@brief returns pointer to a buffer which contains received data
//@return pointer to received data buffer
//
uint8_t* esp8266GetRxBuffer(void);

//
//@brief send raw command
//@param  rawCmd - raw command to be sent
//@param  respStr - string sent by esp8266 on response of rawCmd
//@return false-failed, true-success
//
bool esp8266CommandRAW(const char* rawCmd, const char*respStr);

//
//@brief send test command
//@return false-failed, true-success
//
bool esp8266CommandAT(void);


//
//@brief reset module
//@return false-failed, true-success
//
bool esp8266CommandRST(void);

//
//@brief get module version
//@return false-failed, true-success
//
bool esp8266CommandGMR(void);


//
//@brief sets Wifi mode
//@return false-failed, true-success
//
bool esp8266CommandCWMODE(Esp8266WifiMode mode);

//
//@brief sets Connection mux
//@param 0-single, 1-multiple
//@return false-failed, true-success
//
bool esp8266CommandCIPMUX(uint8_t enable);


//
//@brief list Access points (AP)
//@param 0-single, 1-multiple
//@return false-failed, true-success
//
bool esp8266CommandCWLAP();


//
//@brief connect(join) to a WIFI AP using ssid and password
//@param  ssid - WPA2 AP ssid
//@param  password - WPA2 AP password
//@return false-failed, true-success
//
bool esp8266CommandCWJAP(const char* ssid, const char* pass);

//
//@brief disconnect from a currently connected WIFI AP
//@return false-failed, true-success
//
bool esp8266CommandCWQAP(void);

//
//@brief configure WIFI access point parameters(AP)
//@param  ssid - WPA2 AP ssid
//@param  password - WPA2 AP password
//@param  channel - channel
//@param  encryptMode - seccurity mode
//@return false-failed, true-success
//
bool esp8266CommandCWSAP(const char* ssid, const char* password, uint8_t channel, uint8_t encryptMode);


//
//@brief get local IP address
//@return false-failed, true-success
//
bool esp8266CommandCIFSR(void);

//
//@brief retrieves all information about connection
//@return false-failed, true-success
//
bool esp8266CommandCIPSTATUS(void);


//
//@brief make TCP connection
//@param  proto - protocol type TCP or UDP supported
//@param  ipAddr - IP address or webpage addr as a string
//@param  portNum - Number of TCP/UDP port
//@return false-failed, true-success
//
bool esp8266CommandCIPSTART(Esp8266Protocol proto, const char* ipAddr, uint16_t portNum);


//
//@brief close TCP connection
//@return false-failed, true-success
//
bool esp8266CommandCIPCLOSE();


//
//@brief send a TCP packet to the connected server
//@param  packet - data packet
//@param  packetLen - data packet len, if 0 - length will be computed by strlen
//@return false-failed, true-success
//
bool esp8266CommandCIPSEND(const char* packet, uint16_t packetLen);

#endif /* ESP8266_H_ */
