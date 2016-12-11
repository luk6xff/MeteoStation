/*
 * esp8266.h
 *
 *  Created on: 20 lis 2016
 *      Author: igbt6
 */

#ifndef ESP8266_H_
#define ESP8266_H_

typedef enum
{
	ESP8266_MODE_CLIENT = 1,
	ESP8266_MODE_AP,
	ESP8266_MODE_AP_AND_CLIENT,
	ESP8266_MODE_INVALID
}Esp8266WifiMode;


void esp8266Init();

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
//@brief make TCP connection
//@param  ipAddr - IP address or webpage addr as a string
//@return false-failed, true-success
//
bool esp8266CommandCIPSTART(const char* ipAddr);


//
//@brief close TCP connection
//@return false-failed, true-success
//
bool esp8266CommandCIPCLOSE();


//
//@brief send a TCP packet to the connected server
//@param  packet - data packet
//@return false-failed, true-success
//
bool esp8266CommandCIPSEND(const char* packet);

#endif /* ESP8266_H_ */
