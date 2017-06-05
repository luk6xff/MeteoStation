/*
 * wifi.h
 *
 *  Created on: 6 maj 2017
 *      Author: igbt6
 */

#ifndef WIFI_H_
#define WIFI_H_

#include <stdbool.h>

typedef enum
{
	WIFI_NOT_CONNECTED,
	WIFI_CONNECTED,
	WIFI_WAIT_FOR_DATA,
}WifiConnectionState;


bool wifiInit(const char* ssid, const char* pass);

//
//@brief sets connection parameters of chosen access point
//@param  ssid - WPA2 AP ssid
//@param  password - WPA2 AP password
//
void wifiSetApParameters(const char* ssid, const char* pass);

bool wifiConnectToAp();

bool wifiDisconnectFromAp();

bool wifiGetCurrentWeather(const char* city);

const char* wifiLastReceivedDataBuffer();

WifiConnectionState wifiGetConnectionStatus();


#endif /* WIFI_H_ */
