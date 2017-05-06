/*
 * wifi.c
 *
 * @brief Module responsible for connection and grabbing weataher data from openweathermap.org via WIFI access point
 *  Created on: 6 maj 2017
 *      Author: igbt6
 */
#include <string.h>

#include "wifi.h"
#include "esp8266.h"
#include "config.h"

#define AP_PARAMS_MAX_LENGTH MAX_CONFIG_PARAM_NAME_LENGTH


const char* openweather_server_name = "api.openweathermap.org";
const char* openweather_forecast_url = "/data/2.5/forecast?units=metric";
const char* openweather_weather_url = "/data/2.5/weather?units=metric";


static char wifi_ap_SSID_buf[AP_PARAMS_MAX_LENGTH];
static char wifi_ap_PASS_buf[AP_PARAMS_MAX_LENGTH];

static WifiConnectionState m_current_state;


//
//forward declarations
//
static char* build_url(const char* request_url, const char* city, const char* openweather_api_key);

bool wifiInit(const char* ssid, const char* pass)
{
	//ESP8266
	esp8266Init();
	if(!ssid || !pass || !esp8266CommandAT())
	{
		return false;
	}
	wifiSetApParameters(ssid, pass);
	esp8266CommandCWMODE(ESP8266_MODE_CLIENT); //set as a client
	m_current_state = WIFI_NOT_CONNECTED;
	return true;
}

void wifiSetApParameters(const char* ssid, const char* pass)
{
	if(!ssid || !pass)
	{
		return;
	}
	memset(wifi_ap_SSID_buf, '\0', sizeof(wifi_ap_SSID_buf));
	memset(wifi_ap_PASS_buf, '\0', sizeof(wifi_ap_PASS_buf));
	memcpy(wifi_ap_SSID_buf, ssid, sizeof(wifi_ap_SSID_buf)-1);
	memcpy(wifi_ap_PASS_buf, pass, sizeof(wifi_ap_PASS_buf)-1);
}

WifiConnectionState wifiGetConnectionStatus()
{
	return m_current_state;
}

bool wifiConnectToAp()
{
	return esp8266CommandCWJAP(wifi_ap_SSID_buf, wifi_ap_PASS_buf);
}

bool wifiDisconnectFromAp()
{
	return esp8266CommandCWQAP();
}

bool wifiGetCurrentWeather(const char* city)
{
	if(!m_current_state == WIFI_CONNECTED)
	{
		return false;
	}
	const char* url = build_url(openweather_weather_url, city, "e95bbbe9f7314ea2a5ca1f60ee138eef");
	return true;
}


static char* build_url(const char* request_url, const char* city, const char* openweather_api_key)
{
	static char request_buffer[256];
	memset(request_buffer, '\0', sizeof(request_buffer));
	uint8_t rlen = strlen(request_url);
	uint8_t clen = strlen(city);
	uint8_t klen = strlen(openweather_api_key);
	uint8_t len = 0;
	memcpy(request_buffer, request_url, rlen);
	len += rlen;
	memcpy(&request_buffer[len], "&q=", 3);
	len += 3;
	memcpy(&request_buffer[len], city, clen);
	len += clen;
	memcpy(&request_buffer[len], "&appid=", 7);
	len += 7;
	memcpy(&request_buffer[len], openweather_api_key, klen);
	return request_url;
}

