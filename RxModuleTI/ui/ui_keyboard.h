/*
 * ui_keyboard.h
 *
 *  Created on: 20 mar 2017
 *      Author: igbt6
 */

#ifndef UI_UI_KEYBOARD_H_
#define UI_UI_KEYBOARD_H_

typedef enum
{
	AlphaNumeric,
	Numeric,
	All
}KeyboardAllowedChars;

bool uiKeyboardInit();

bool uiKeyboardCreate(char* param, Screens prevScreen, KeyboardAllowedChars charsAllowed,
					  const char* retMsgBoxTitle, const char* retMsgBoxContent,
					  void(*exitKeyboardCb)(const Screens prevWidget, bool save));

void uiKeyboardSetAllowedCharsType(KeyboardAllowedChars type);

#endif /* UI_UI_KEYBOARD_H_ */
