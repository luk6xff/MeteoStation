/*
 * ui_keyboard.c
 *
 *  Created on: 20 mar 2017
 *      Author: igbt6
 */
#include "ui_common.h"
#include "ui_keyboard.h"
#include "ui_message_box.h"

//*****************************************************************************
// The canvas widget acting as a Keyboard
//*****************************************************************************

// Keyboard cursor blink rate in ms.
#define KEYBOARD_BLINK_RATE_MS	500
#define KEYBOARD_TEXT_CHARS_NUM	KEYBOARD_MAX_TEXT_LEN
#define KEYBOARD_NUMERIC_TEXT_CHARS_NUM 3

static bool m_keyboardActive = false;

static tCanvasWidget m_keyboardBackground;

// The current string pointer for the keyboard.
static char *m_keyboardString;

// The current string index for the keyboard.
static uint32_t m_keyboardStringIdx;

// The current string width for the keyboard in pixels.
static int32_t m_keyboardStringWidth;

// A string which contains the content of the keyboard text view window.
static char m_keyboardTempString[KEYBOARD_TEXT_CHARS_NUM] = {'\0'};

// Pointer to global drawing context
static tContext* m_drawingCtx = NULL;

// Previous widget which shall be redraw after keyboard exit
static Screens m_previousScreen;

// Allowed chars type in text view - Alpha numeric by default
static KeyboardAllowedChars m_allowedChars = AlphaNumeric;

// Temporary buffer for numeric chars, i bet they will be no longer than 3 chars eg. 674
static char m_numericCharsBuf[KEYBOARD_NUMERIC_TEXT_CHARS_NUM];

// Params which describe exit msgBox
static const char* m_exitKeyboardMsgBoxTitle;
static const char* m_exitKeyboardMsgContent;


//*****************************************************************************
//Forward local methods declarations
//*****************************************************************************
static void onKeyEvent(tWidget *widget, uint32_t key, uint32_t event);
static bool isKeyValid(char key);
static void (*onExitKeyboardCb)(const Screens previousWidget, bool save);

// The keyboard widget used by the application.
static Keyboard(m_keyboard, &m_keyboardBackground, 0, 0,
		 &g_ILI9320, 8, 90, 300, 140,
         KEYBOARD_STYLE_FILL | //KEYBOARD_STYLE_AUTO_REPEAT// |
		 KEYBOARD_STYLE_PRESS_NOTIFY | KEYBOARD_STYLE_BG,
         ClrBlack, ClrGray, ClrDarkGray, ClrGray, ClrBlack, g_psFontCmss14,
         100, 100, NUM_KEYBOARD_US_ENGLISH, g_psKeyboardUSEnglish, onKeyEvent);

// The keyboard text entry area.
static Canvas(m_keyboardTextView, &m_keyboardBackground, &m_keyboard, 0,
	   &g_ILI9320, BG_MIN_X, BG_MIN_Y,
       BG_MAX_X - BG_MIN_X, 60,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_LEFT,
       ClrBlack, ClrWhite, ClrWhite, g_psFontCmss24, m_keyboardTempString, 0 ,0 );

// The full background for the keyboard when it is takes over the screen.
static Canvas(m_keyboardBackground, WIDGET_ROOT, 0, &m_keyboardTextView,
	   &g_ILI9320, BG_MIN_X, BG_MIN_Y,
       BG_MAX_X - BG_MIN_X, BG_MAX_Y - BG_MIN_Y,
       CANVAS_STYLE_FILL, ClrBlack, ClrWhite, ClrWhite, 0, 0, 0 ,0 );


//*****************************************************************************
// @brief Handles when a key is pressed on the keyboard.
//*****************************************************************************
static void onKeyEvent(tWidget *widget, uint32_t key, uint32_t event)
{
    switch(key)
    {
        // Look for a backspace key press.
        case UNICODE_BACKSPACE:
        {
            if(event == KEYBOARD_EVENT_PRESS)
            {
                if(m_keyboardStringIdx != 0)
                {
                    m_keyboardStringIdx--;
                    m_keyboardTempString[m_keyboardStringIdx] = '\0';
                }

                WidgetPaint((tWidget *)&m_keyboardTextView);

                // Save the pixel width of the current string.
                m_keyboardStringWidth = GrStringWidthGet(m_drawingCtx, m_keyboardTempString, 40);
            }
            break;
        }
        // Look for an enter/return key press.  This will exit the keyboard and
        // return to the last active screen.
        case UNICODE_RETURN:
        {
            if(event == KEYBOARD_EVENT_PRESS)
            {
            	m_keyboardActive = false;
            	WidgetRemove((tWidget*)&m_keyboardBackground);
                //WidgetMessageQueueProcess();
            	bool save = uiMessageBoxCreate(m_exitKeyboardMsgBoxTitle, m_exitKeyboardMsgContent);
            	if(save)
            	{
            	    //copy changed content to param
                    if(m_allowedChars == Numeric)
                    {
                    	memcpy(m_keyboardString, m_keyboardTempString, KEYBOARD_NUMERIC_TEXT_CHARS_NUM*sizeof(char));
                    }
                    else
                    {
                    	memcpy(m_keyboardString, m_keyboardTempString, KEYBOARD_TEXT_CHARS_NUM*sizeof(char));
                    }
            	}
            	m_keyboard.ui32Flags = 0; //clear all flags of the widget
            	(*onExitKeyboardCb)(m_previousScreen, save);
            }
            break;
        }
        // If the key is not special then update the text string.
        default:
        {
            if(event == KEYBOARD_EVENT_PRESS)
            {
                // Set the string to the current string to be updated.
                //if(m_keyboardStringIdx == 0)
                //{
                //    CanvasTextSet(&m_keyboardTextView, "\0");
                //}
                if(isKeyValid(key))
                {
                    m_keyboardTempString[m_keyboardStringIdx] = (char)key;
                    if(m_allowedChars == Numeric)
                    {
                    	if(m_keyboardStringIdx < KEYBOARD_NUMERIC_TEXT_CHARS_NUM)
						{
							m_keyboardStringIdx++;
							m_keyboardTempString[m_keyboardStringIdx] = '\0';
						}
                    }
                    else
                    {
                    	if(m_keyboardStringIdx < KEYBOARD_TEXT_CHARS_NUM)
						{
							m_keyboardStringIdx++;
							m_keyboardTempString[m_keyboardStringIdx] = '\0';
						}
                    }

                    WidgetPaint((tWidget *)&m_keyboardTextView);
                    // Save the pixel width of the current string.
                    m_keyboardStringWidth = GrStringWidthGet(m_drawingCtx, m_keyboardTempString, 40);
                }
            }
            break;
        }
    }
}
//*****************************************************************************
//
// @brief checks if pressed key is valid (based on m_allowedChars type)
//
//*****************************************************************************
static bool isKeyValid(char key)
{
	bool ret = false;
	switch(m_allowedChars)
	{
	case AlphaNumeric:
		if((('0' <= key) && (key <= '9')) ||
		   (('a' <= key) && (key <= 'z')) ||
		   (('A' <= key) && (key <= 'Z')))
		{
			ret = true;
		}
		break;

	case Numeric:
		if(('0' <= key) && (key <= '9'))
		{
			ret = true;
		}
		break;

	case All:
		ret = true;
		break;

	default:
		break;
	}
	return ret;
}
//*****************************************************************************
//
// @brief handle keyboard updates.
//
//*****************************************************************************
static void handleKeyboardCursor(void)
{
    // Nothing to do if the keyboard is not active.
    if(!m_keyboardActive)
    {
        return;
    }
    // The cursor blink state; false-disabled, true-enabled.
    static bool m_keyboardCursorBlinkState = false;
    if(m_keyboardCursorBlinkState)
    {
        GrContextForegroundSet(m_drawingCtx, ClrBlack);
        m_keyboardCursorBlinkState = false;
    }
    else
    {
        GrContextForegroundSet(m_drawingCtx, ClrWhite);
        m_keyboardCursorBlinkState = true;
    }
    // Draw the cursor only if it is time.
    GrLineDrawV(m_drawingCtx, BG_MIN_X+m_keyboardStringWidth,
    			BG_MIN_Y+20, BG_MIN_Y+40);
}


//publics
//*****************************************************************************
// @brief Inits the keyboard instance
//*****************************************************************************
bool uiKeyboardInit()
{
	m_drawingCtx = uiGetMainDrawingContext();
	uiRegisterTimerCb(handleKeyboardCursor, KEYBOARD_BLINK_RATE_MS); //register cursor blinker handler
	return true;
}

//*****************************************************************************
// @brief creates and draws the keyboard widget on the screen
//*****************************************************************************
bool uiKeyboardCreate(char* param, Screens prevScreen, KeyboardAllowedChars charsAllowed,
					  const char* retMsgBoxTitle, const char* retMsgBoxContent,
					  void(*exitKeyboardCb)(const Screens prevWidget, bool save))
{
	m_keyboardString = param;
	m_previousScreen = prevScreen;
	m_allowedChars = charsAllowed;
	m_exitKeyboardMsgContent = retMsgBoxContent;
	onExitKeyboardCb = exitKeyboardCb;

	if(m_allowedChars == Numeric)
	{
		memcpy(m_keyboardTempString, m_numericCharsBuf, KEYBOARD_NUMERIC_TEXT_CHARS_NUM*sizeof(char));
	}
	else // Others
	{
		// Set the initial string to the param value.
		memcpy(m_keyboardTempString, m_keyboardString, KEYBOARD_TEXT_CHARS_NUM*sizeof(char));
	}
    //update length and idx
    m_keyboardStringIdx = strlen(m_keyboardTempString);
    m_keyboardStringWidth = GrStringWidthGet(m_drawingCtx, m_keyboardTempString, 40);


    WidgetAdd(WIDGET_ROOT, (tWidget*)&m_keyboardBackground);
    WidgetPaint(WIDGET_ROOT);
    CanvasTextSet(&m_keyboardTextView, m_keyboardTempString);
    WidgetMessageQueueProcess(); //draw the actual keyboard on the screen

    //mark keyboard already active
	m_keyboardActive = true;
	return true;
}

void uiKeyboardSetAllowedCharsType(KeyboardAllowedChars type)
{
	m_allowedChars = type;
}
