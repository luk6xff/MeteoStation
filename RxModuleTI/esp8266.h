/*
 * esp8266.h
 *
 *  Created on: 20 lis 2016
 *      Author: igbt6
 */

#ifndef ESP8266_H_
#define ESP8266_H_


void esp8266Init();

bool esp8266CommandAT(void);

bool esp8266CommandRST(void);

#endif /* ESP8266_H_ */
