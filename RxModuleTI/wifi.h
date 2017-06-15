/*
 * wifi.h
 *
 *  Created on: 6 maj 2017
 *      Author: igbt6
 */

#ifndef WIFI_H_
#define WIFI_H_

#include <stdbool.h>
#include "time_lib.h"

typedef enum
{
	WIFI_NOT_CONNECTED,
	WIFI_CONNECTED,
	WIFI_TRANSMISSION_CREATED,
	WIFI_TRANSMISSION_ENDED,
}WifiConnectionState;


typedef struct
{
	int temperature;
	int pressure;
	unsigned int humidity;
	int wind_speed;
	int wind_direction;
	unsigned int current_time;
	unsigned int sunrise_time;
	unsigned int sunset_time;
	int weather_cond_code[3];   //max 3 codes
	int is_valid;
}WifiWeatherDataModel;

bool wifiInit(const char* ssid, const char* pass);

//
//@brief sets connection parameters of chosen access point
//@param  ssid - WPA2 AP ssid
//@param  password - WPA2 AP password
//
void wifiSetApParameters(const char* ssid, const char* pass);

bool wifiConnectToAp();

bool wifiDisconnectFromAp();

bool wifiFetchCurrentWeather(const char* city);

bool wifiFetchCurrentNtpTime();

bool wifiCheckApConnectionStatus();

WifiConnectionState wifiGetConnectionStatus();

WifiWeatherDataModel wifiGetWeatherResultData();

timeData_t wifiGetTimeResultData();

const char* wifiSsidParam();
const char* wifiPassParam();


#endif /* WIFI_H_ */
