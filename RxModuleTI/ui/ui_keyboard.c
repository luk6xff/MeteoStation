/*
 * ui_keyboard.c
 *
 *  Created on: 20 mar 2017
 *      Author: igbt6
 */
#include "ui_common.h"
#include "ui_keyboard.h"

//*****************************************************************************
// The canvas widget acting as a Keyboard
//*****************************************************************************

// Keyboard cursor blink rate.
#define KEYBOARD_BLINK_RATE     100

static tCanvasWidget m_keyboardBackground;

// The current string pointer for the keyboard.
static char *m_keyboardString;

// The current string index for the keyboard.
static uint32_t m_keyboardStringIdx;

// A place holder string used when nothing is being displayed on the keyboard.
static const char m_keyboardTempString = 0;

// The current string width for the keyboard in pixels.
static int32_t m_keyboardStringWidth;

// The cursor blink counter.
static volatile uint32_t m_keyboardCursorDelay;

static tContext* m_drawingCtx = NULL;

// The keyboard widget used by the application.
static Keyboard(m_keyboard, &m_keyboardBackground, 0, 0,
		 &g_ILI9320, 8, 90, 300, 140,
         KEYBOARD_STYLE_FILL | KEYBOARD_STYLE_AUTO_REPEAT |
         KEYBOARD_STYLE_PRESS_NOTIFY | KEYBOARD_STYLE_RELEASE_NOTIFY |
         KEYBOARD_STYLE_BG,
         ClrBlack, ClrGray, ClrDarkGray, ClrGray, ClrBlack, g_psFontCmss14,
         100, 100, NUM_KEYBOARD_US_ENGLISH, g_psKeyboardUSEnglish, onKeyEvent);

// The keyboard text entry area.
static Canvas(m_keyboardTextView, &m_keyboardBackground, &m_keyboard, 0,
	   &g_ILI9320, BG_MIN_X, BG_MIN_Y,
       BG_MAX_X - BG_MIN_X, 60,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_LEFT,
       ClrBlack, ClrWhite, ClrWhite, g_psFontCmss24, &m_keyboardTempString, 0 ,0 );

// The full background for the keyboard when it is takes over the screen.
static Canvas(m_keyboardBackground, WIDGET_ROOT, 0, &m_keyboardTextView,
	   &g_ILI9320, BG_MIN_X, BG_MIN_Y,
       BG_MAX_X - BG_MIN_X, BG_MAX_Y - BG_MIN_Y,
       CANVAS_STYLE_FILL, ClrBlack, ClrWhite, ClrWhite, 0, 0, 0 ,0 );





//init
bool uiKeyboardInit()
{
	m_drawingCtx = uiGetMainDrawingContext();
	return true;
}
