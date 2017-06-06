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
#include "tiny-json.h"

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
static char* wifi_build_url(const char* request_url, const char* city, const char* openweather_api_key);
static bool wifi_parse_weather_response(const uint8_t* resp_buf, uint16_t buf_len);

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
	if (!esp8266CommandRST())
	{
		return false;
	}
	if (esp8266CommandCWJAP(wifi_ap_SSID_buf, wifi_ap_PASS_buf))
	{
		m_current_state = WIFI_CONNECTED;
		return true;
	}
	return false;
}

bool wifiDisconnectFromAp()
{
	if (!esp8266CommandAT())
	{
		return false;
	}
	if (esp8266CommandCWQAP())
	{
		m_current_state = WIFI_NOT_CONNECTED;
		return true;
	}
	return false;
}

bool wifiGetCurrentWeather(const char* city)
{
	if(!m_current_state == WIFI_CONNECTED)
	{
		return false;
	}

	if (!esp8266CommandAT())
	{
		return false;
	}

	if (!esp8266CommandCIPSTART(openweather_server_name))
	{
		return false;
	}
	const char* url = wifi_build_url(openweather_weather_url, city, "e95bbbe9f7314ea2a5ca1f60ee138eef");

	if (!esp8266CommandCIPSEND(url))
	{
		return false;
	}
	delay_ms(100); //to finish download
	return wifi_parse_weather_response(esp8266GetRxBuffer(), ESP8266_RX_BUF_SIZE);
}

const char* wifiSsidParam()
{
	return wifi_ap_SSID_buf;
}

const char* wifiPassParam()
{
	return wifi_ap_PASS_buf;
}


//
//@brief status from ESP8266
//@return success or not
//@note All possible values returned by AT+CIPSTATUS command:
//		2 : ESP8266 station connected to an AP and has obtained IP
//		3 : ESP8266 station created a TCP or UDP transmission
//		4 : the TCP or UDP transmission of ESP8266 station disconnected
//		5 : ESP8266 station did NOT connect to an AP
//
bool wifiCheckApConnectionStatus()
{
	if (!esp8266CommandCIPSTATUS())
	{
		return false;
	}
	const uint8_t *const p = strstr(esp8266GetRxBuffer(),"STATUS:");
	if (!p)
    {
    	return false;
    }

    uint8_t status = p[7] - '0';
    switch(status)
    {
		case 2:
			m_current_state = WIFI_CONNECTED;
			break;
		case 3:
			m_current_state = WIFI_TRANSMISSION_CREATED;
			break;
		case 4:
			m_current_state = WIFI_TRANSMISSION_ENDED;
			break;
		case 5:
			m_current_state = WIFI_NOT_CONNECTED;
			break;
		default:
			break;
    }
    return true;
}


///private helpers///
static bool wifi_parse_weather_response(const uint8_t* resp_buf, uint16_t buf_len)
{
	if (!resp_buf)
	{
		return false;
	}
	const uint8_t * const p = strstr(resp_buf, "+IPD,");
	const uint8_t * const r = strstr(resp_buf, "CLOSED");
    if (!p || !r)
    {
    	return false;
    }
    uint16_t len = 0;
    uint8_t ch = '\0';
    size_t shift;
    for (shift = 5; shift < buf_len && ch != ':'; ++shift )
    {
    	ch = p[shift];
    	if ((shift -5) < 3)
    	{
    		len = len*10 + ch - '0';
    	}
    }
    json_t mem[120];
    json_t const* json = json_create(&p[shift], mem, sizeof mem / sizeof *mem );
    if ( !json ) {
    	return false;
    }
    json_t const* main = json_getProperty( json, "main" );
    if ( !main || JSON_TEXT != json_getType( main ) ) {
    	return false;
    }
	return true;
}


static char* wifi_build_url(const char* request_url, const char* city, const char* openweather_api_key)
{
	//"GET /data/2.5/weather?q=NowySacz&appid=e95bbbe9f7314ea2a5ca1f60ee138eef\r\n"
	static char request_buffer[256];
	memset(request_buffer, '\0', sizeof(request_buffer));

	uint8_t rlen = strlen(request_url);
	uint8_t clen = strlen(city);
	uint8_t klen = strlen(openweather_api_key);
	uint8_t len = 4;
	memcpy(request_buffer, "GET ", len);
	memcpy(&request_buffer[len], request_url, rlen);
	len += rlen;
	memcpy(&request_buffer[len], "&q=", 3);
	len += 3;
	memcpy(&request_buffer[len], city, clen);
	len += clen;
	memcpy(&request_buffer[len], "&appid=", 7);
	len += 7;
	memcpy(&request_buffer[len], openweather_api_key, klen);
	return request_buffer;
}

