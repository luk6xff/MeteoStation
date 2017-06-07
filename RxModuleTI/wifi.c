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
#include "delay.h"
#include "tiny-json.h"

#define AP_PARAMS_MAX_LENGTH CONFIG_MAX_PARAM_NAME_LENGTH


const char* openweather_server_name = "api.openweathermap.org";
const char* openweather_forecast_url = "/data/2.5/forecast?units=metric";
const char* openweather_weather_url = "/data/2.5/weather?units=metric";


static char wifi_ap_SSID_buf[AP_PARAMS_MAX_LENGTH];
static char wifi_ap_PASS_buf[AP_PARAMS_MAX_LENGTH];

static WifiConnectionState m_current_state;
static WifiWeatherDataModel m_last_result;

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

WifiConnectionState wifiGetConnectionStatus()
{
	return m_current_state;
}

WifiWeatherDataModel wifiGetWeatherResultData()
{
	return m_last_result;
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
    	goto FAIL;
	}
	const uint8_t * const p = strstr(resp_buf, "+IPD,");
	const uint8_t * const r = strstr(resp_buf, "CLOSED");
    if (!p || !r)
    {
    	goto FAIL;
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
    json_t pool[50];
    json_t const* parent = json_create(&p[shift], pool, sizeof pool / sizeof *pool );
    if (!parent)
    {
    	goto FAIL;
    }

    json_t const* main_ = json_getProperty(parent, "main");
    if (!main_ || JSON_OBJ != json_getType(main_))
    {
    	goto FAIL;
    }
    else
    {
        json_t const* temp = json_getProperty(main_, "temp");
        if ( !temp || JSON_INTEGER != json_getType(temp))
        {
        	goto FAIL;
        }
        else
        {
        	m_last_result.temperature = (int)json_getInteger(temp);
        }

        json_t const* pressure = json_getProperty(main_, "pressure");
        if ( !pressure || JSON_INTEGER != json_getType(pressure))
        {
        	goto FAIL;
        }
        else
        {
        	m_last_result.pressure = (int)json_getInteger(pressure);
        }

        json_t const* humidity = json_getProperty(main_, "humidity");
        if ( !humidity || JSON_INTEGER != json_getType(humidity))
        {
        	goto FAIL;
        }
        else
        {
        	m_last_result.humidity = (int)json_getInteger(humidity);
        }
    }
    //wind
    json_t const* wind = json_getProperty(parent, "wind");
    if (!wind || JSON_OBJ != json_getType(wind))
    {
    	goto FAIL;
    }
    else
    {
        json_t const* speed = json_getProperty(wind, "speed");
        if ( !speed || JSON_REAL != json_getType(speed))
        {
        	goto FAIL;
        }
        else
        {
        	m_last_result.wind_speed = (int)json_getInteger(speed);
        }

        json_t const* deg = json_getProperty(wind, "deg");
        if ( !deg || JSON_INTEGER != json_getType(deg))
        {
        	goto FAIL;
        }
        else
        {
        	m_last_result.wind_direction = (int)json_getInteger(deg);
        }
    }

    //weather condition
	json_t const* weather = json_getProperty(parent, "weather");
	if (!weather || JSON_ARRAY != json_getType(weather))
	{
		goto FAIL;
	}
	else
	{
	   json_t const* weather_cond;
	   uint8_t idx = 0;
	   for(weather_cond = json_getChild(weather); weather_cond != 0; weather_cond = json_getSibling(weather_cond))
	   {
		   if (JSON_OBJ == json_getType(weather_cond))
		   {

				json_t const* weather_id = json_getProperty(weather_cond, "id");
		        if ( !weather_id || JSON_INTEGER != json_getType(weather_id))
		        {
		        	goto FAIL;
		        }
		        else
		        {
		        	m_last_result.weather_cond_code[idx++] = (int)json_getInteger(weather_id);
		        }
		   }
		   else
		   {
			   goto FAIL;
		   }
		   if (idx > 2)
		   {
			   break;
		   }
	   }
	}
	return true;

FAIL:
	return false;
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

