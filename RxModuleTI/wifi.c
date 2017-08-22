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


static const char* openweather_server_name = "api.openweathermap.org";
static const uint16_t openweather_server_port = 80;
static const char* openweather_forecast_url = "/data/2.5/forecast?units=metric";
static const char* openweather_weather_url = "/data/2.5/weather?units=metric";
static const char* openweather_weather_api_key = "e95bbbe9f7314ea2a5ca1f60ee138eef";

static const char* ntp_servers_name[] = {"pool.ntp.org", "europe.pool.ntp.org", "pl.pool.ntp.org", "time.nist.gov"};
static uint8_t ntp_servers_name_idx = 0;
static const uint16_t ntp_server_port = 123;
#define NTP_PACKET_SIZE  48 // NTP time is in the first 48 bytes of message


static char wifi_ap_SSID_buf[AP_PARAMS_MAX_LENGTH];
static char wifi_ap_PASS_buf[AP_PARAMS_MAX_LENGTH];

static WifiConnectionState m_current_state;
static WifiWeatherDataModel m_last_result;

//
//forward declarations
//
static char* wifi_weather_build_url(const char* request_url, const char* city, const char* openweather_api_key);
static bool wifi_weather_parse_response(const uint8_t* resp_buf, uint16_t buf_len);
static void wifi_weather_set_result_invalid();
static bool wifi_ntp_parse_response(const uint8_t* resp_buf, uint16_t buf_len, timeData_t* time);
static char* wifi_ntp_build_url();


bool wifiInit(const char* ssid, const char* pass)
{
	wifi_weather_set_result_invalid();
	//ESP8266
	esp8266Init();
	if (!ssid || !pass || strcmp(ssid, "default") == 0 || strcmp(pass, "default") == 0)
	{
		return false;
	}
	if (!esp8266CommandAT())
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
	if (!ssid || !pass)
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
	if (m_current_state == WIFI_CONNECTED)
	{
		goto ok;
	}

	if (esp8266CommandRST())
	{
		if (esp8266CommandCWJAP(wifi_ap_SSID_buf, wifi_ap_PASS_buf))
		{
			m_current_state = WIFI_CONNECTED;
			goto ok;
		}
	}
	return false;
ok:
	return true;
}

bool wifiDisconnectFromAp()
{
	if (!esp8266CommandAT())
	{
		goto fail;
	}
	if (!esp8266CommandCWQAP())
	{
		goto fail;
	}
	m_current_state = WIFI_NOT_CONNECTED;

	return true;
fail:
	return false;
}

bool wifiFetchCurrentWeather(const char* city)
{
	wifi_weather_set_result_invalid();
	if (!m_current_state == WIFI_CONNECTED)
	{
		goto fail;
	}

	if (!esp8266CommandAT())
	{
		goto fail;
	}

	esp8266CommandCIPCLOSE();

	if (!esp8266CommandCIPSTART(ESP8266_PROTOCOL_TCP, openweather_server_name, openweather_server_port))
	{
		goto fail;
	}
	const char* url = wifi_weather_build_url(openweather_weather_url, city, openweather_weather_api_key);

	if (!esp8266CommandCIPSEND(url, 0))
	{
		goto fail;
	}
	delay_ms(100); //to finish download
	if (!wifi_weather_parse_response(esp8266GetRxBuffer(), ESP8266_RX_BUF_SIZE))
	{
		goto fail;
	}

	return true;

fail:
	return false;
}

timeData_t wifiFetchCurrentNtpTime()
{
	timeData_t timeRetrieved = 0;
	if (!m_current_state == WIFI_CONNECTED)
	{
		goto fail;
	}

	if (!esp8266CommandAT())
	{
		goto fail;
	}

	esp8266CommandCIPCLOSE();

	if (!esp8266CommandCIPSTART(ESP8266_PROTOCOL_UDP, ntp_servers_name[ntp_servers_name_idx], ntp_server_port))
	{
		goto fail;
	}

	if (!esp8266CommandCIPSEND(wifi_ntp_build_url(), NTP_PACKET_SIZE))
	{
		goto fail;
	}

	delay_ms(100); //just wait a short while

	if (!wifi_ntp_parse_response(esp8266GetRxBuffer(), ESP8266_RX_BUF_SIZE, &timeRetrieved))
	{
		goto fail;
	}

	return timeRetrieved;

fail:
	ntp_servers_name_idx = (ntp_servers_name_idx + 1) % sizeof(ntp_servers_name); //if sth does not work change ntp server for next check
	return 0;
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
		goto fail;
	}
	const uint8_t *const p = strstr(esp8266GetRxBuffer(),"STATUS:");
	if (!p)
    {
    	goto fail;
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
fail:
	return false;
}

//
//private helpers
//
static bool wifi_weather_parse_response(const uint8_t* resp_buf, uint16_t buf_len)
{
	if (!resp_buf)
	{
    	goto fail;
	}
	const uint8_t * const p = strstr(resp_buf, "+IPD,");
	const uint8_t * const r = strstr(resp_buf, "CLOSED");
    if (!p || !r)
    {
    	goto fail;
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

    //create no more than 50 fields expected
    json_t pool[50];
    json_t const* parent = json_create(&p[shift], pool, sizeof pool / sizeof *pool );
    if (!parent)
    {
    	goto fail;
    }

    json_t const* main_ = json_getProperty(parent, "main");
    if (!main_ || JSON_OBJ != json_getType(main_))
    {
    	goto fail;
    }
    else
    {
        json_t const* temp = json_getProperty(main_, "temp");
        if ( !temp || JSON_INTEGER != json_getType(temp))
        {
        	goto fail;
        }
        else
        {
        	m_last_result.temperature = (int)json_getInteger(temp);
        }

        json_t const* pressure = json_getProperty(main_, "pressure");
        if ( !pressure || JSON_INTEGER != json_getType(pressure))
        {
        	goto fail;
        }
        else
        {
        	m_last_result.pressure = (int)json_getInteger(pressure);
        }

        json_t const* humidity = json_getProperty(main_, "humidity");
        if ( !humidity || JSON_INTEGER != json_getType(humidity))
        {
        	goto fail;
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
    	//just skip
    }
    else
    {
        json_t const* speed = json_getProperty(wind, "speed");
        if ( !speed || JSON_REAL != json_getType(speed))
        {
        	//just skip
        }
        else
        {
        	m_last_result.wind_speed = (int)json_getInteger(speed);
        }

        json_t const* deg = json_getProperty(wind, "deg");
        if ( !deg || JSON_INTEGER != json_getType(deg))
        {
        	//just skip
        }
        else
        {
        	m_last_result.wind_direction = (int)json_getInteger(deg);
        }
    }

    //current time
    json_t const* current_time = json_getProperty(parent, "dt");
    if (!current_time || JSON_INTEGER != json_getType(current_time))
    {
    	goto fail;
    }
    else
    {
    	m_last_result.current_time = (unsigned int)json_getInteger(current_time);
    }

    //sunrise/sunset times
    json_t const* sys = json_getProperty(parent, "sys");
    if (!sys || JSON_OBJ != json_getType(sys))
    {
    	goto fail;
    }
    else
    {
        json_t const* sunrise_time = json_getProperty(sys, "sunrise");
        if ( !sunrise_time || JSON_INTEGER != json_getType(sunrise_time))
        {
        	goto fail;
        }
        else
        {
        	m_last_result.sunrise_time = (unsigned int)json_getInteger(sunrise_time);
        }

        json_t const* sunset_time = json_getProperty(sys, "sunset");
        if ( !sunset_time || JSON_INTEGER != json_getType(sunset_time))
        {
        	goto fail;
        }
        else
        {
        	m_last_result.sunset_time = (unsigned int)json_getInteger(sunset_time);
        }
    }

    //weather condition
	json_t const* weather = json_getProperty(parent, "weather");
	if (!weather || JSON_ARRAY != json_getType(weather))
	{
		goto fail;
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
		        	goto fail;
		        }
		        else
		        {
		        	m_last_result.weather_cond_code[idx++] = (int)json_getInteger(weather_id);
		        }
		   }
		   else
		   {
			   goto fail;
		   }
		   if (idx > 2)
		   {
			   break;
		   }
	   }
	}
	m_last_result.is_valid = 0;
	return true;

fail:
	m_last_result.is_valid = -1;
	return false;
}


static char* wifi_weather_build_url(const char* request_url, const char* city, const char* openweather_api_key)
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
	len += klen;
	memcpy(&request_buffer[len], "\r\n", 2);
	return request_buffer;
}

static void wifi_weather_set_result_invalid()
{
    //reset last results
    memset(&m_last_result, -1, sizeof(WifiWeatherDataModel));
}

static char* wifi_ntp_build_url()
{
	static char ntp_request_buffer[NTP_PACKET_SIZE];
	memset(ntp_request_buffer, '\0', sizeof(ntp_request_buffer));
	// Initialize values needed to form NTP request
	// (see https://tools.ietf.org/html/rfc5905 above for details on the packets)
	ntp_request_buffer[0] = 0xE3;   // LI, Version, Mode
	ntp_request_buffer[1] = 0;     // Stratum, or type of clock
	ntp_request_buffer[2] = 6;     // Polling Interval
	ntp_request_buffer[3] = 0xEC;  // Peer Clock Precision
	// 8 bytes of zero for Root Delay & Root Dispersion
	ntp_request_buffer[12] = 49;
	ntp_request_buffer[13] = 0x4E;
	ntp_request_buffer[14] = 49;
	ntp_request_buffer[15] = 52;
	//ntp_request_buffer[0] = 0x1b;
	return ntp_request_buffer;
}

static bool wifi_ntp_parse_response(const uint8_t* resp_buf, uint16_t buf_len, timeData_t* time)
{
	if (!resp_buf)
	{
    	goto fail;
	}
	const char* pattern = "+IPD,";
	const uint8_t * const p = strstr(resp_buf, pattern);
    if (!p)
    {
    	goto fail;
    }
    uint16_t len = 0;
    uint8_t ch = '\0';
    size_t shift;
    size_t pattern_idx = 0;
    for (shift = 0; shift < buf_len && ch != ':'; ++shift )
    {
    	ch = p[shift];
    	if (ch == pattern[pattern_idx])
    	{
    		pattern_idx++;
    	}
    	else
    	{
    		if (pattern_idx < 5)
    		{
    			pattern_idx = 0;
    		}
    		else if (pattern_idx == 5 && ch != ':') // 5 as a strlen of pattern = "+IPD,"
    		{
        		len = len*10 + ch - '0';
    		}
    	}
    }
    if (len != NTP_PACKET_SIZE)
    {
    	goto fail;
    }

    timeData_t secs_since_1900 = 0;
    const uint32_t TIME_DIFF_1900_TO_1970 = 2208988800;
    secs_since_1900 |= (uint64_t)p[shift + 40] << 24;
    secs_since_1900 |= (uint64_t)p[shift + 41] << 16;
    secs_since_1900 |= (uint64_t)p[shift + 42] << 8;
    secs_since_1900 |= (uint64_t)p[shift + 43];

    *time = (secs_since_1900 - TIME_DIFF_1900_TO_1970); // convert NTP time to Unix time
	return true;

fail:
	return false;
}
