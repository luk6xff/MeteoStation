#include "ui_common.h"
#include "ui_message_box.h"

/*sizes of message box*/
#define MSGBOX_UP_LEFT_CORNER_X BG_MIN_X+40
#define MSGBOX_UP_LEFT_CORNER_Y BG_MIN_Y+40
#define MSGBOX_WIDTH_X (BG_MAX_X-(BG_MIN_X+(2*40)))
#define MSGBOX_HEIGHT_Y (BG_MAX_Y-(BG_MIN_Y+(2*40)))

#define MSGBOX_YES_UP_LEFT_CORNER_X (MSGBOX_UP_LEFT_CORNER_X+10)
#define MSGBOX_YES_UP_LEFT_CORNER_Y (MSGBOX_UP_LEFT_CORNER_Y+MSGBOX_HEIGHT_Y-10-MSGBOX_YES_HEIGHT_Y)
#define MSGBOX_YES_WIDTH_X (70)
#define MSGBOX_YES_HEIGHT_Y (40)

#define MSGBOX_NO_UP_LEFT_CORNER_X (MSGBOX_UP_LEFT_CORNER_X+MSGBOX_WIDTH_X-10-MSGBOX_NO_WIDTH_X)
#define MSGBOX_NO_UP_LEFT_CORNER_Y (MSGBOX_UP_LEFT_CORNER_Y+MSGBOX_HEIGHT_Y-10-MSGBOX_YES_HEIGHT_Y)
#define MSGBOX_NO_WIDTH_X (70)
#define MSGBOX_NO_HEIGHT_Y (40)

#define MSGBOX_TITLE_HEIGHT_Y (24)
/*sizes of message box - end*/

static tContext* m_drawingCtx = NULL;
static tPushButtonWidget m_yesButton;
static tPushButtonWidget m_noButton;
static tCanvasWidget m_msgBoxBackground;
static void onYesPressButton();
static void onNoPressButton();
static bool m_buttonPressed = false;
static bool m_buttonPressResult = false;

static RectangularButton(m_yesButton, &m_msgBoxBackground, 0, 0,
				  &g_ILI9320, MSGBOX_YES_UP_LEFT_CORNER_X, MSGBOX_YES_UP_LEFT_CORNER_Y,
				  MSGBOX_YES_WIDTH_X, MSGBOX_YES_HEIGHT_Y,
				  PB_STYLE_FILL | PB_STYLE_TEXT | PB_STYLE_OUTLINE,
				  ClrGreen, ClrRed, ClrSilver, ClrWhite, g_psFontCm16,
				  "Yes", 0, 0, 0 ,0 , onYesPressButton);

static RectangularButton(m_noButton, &m_msgBoxBackground, &m_yesButton, 0,
				  &g_ILI9320, MSGBOX_NO_UP_LEFT_CORNER_X, MSGBOX_NO_UP_LEFT_CORNER_Y,
				  MSGBOX_NO_WIDTH_X, MSGBOX_NO_HEIGHT_Y,
				  PB_STYLE_FILL | PB_STYLE_TEXT | PB_STYLE_OUTLINE,
				  ClrGreen, ClrRed, ClrSilver, ClrWhite, g_psFontCm16,
				  "No", 0, 0, 0 ,0 , onNoPressButton);
/*Main canvases of message box*/
static Canvas(m_msgBoxBackground, WIDGET_ROOT, 0, &m_noButton,
	   &g_ILI9320, MSGBOX_UP_LEFT_CORNER_X, MSGBOX_UP_LEFT_CORNER_Y,
	   MSGBOX_WIDTH_X, MSGBOX_HEIGHT_Y,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT_RIGHT |
       CANVAS_STYLE_TEXT_TOP, BG_COLOR_MAIN, ClrWhite, ClrBlueViolet, 0,
       0, 0, 0);

static void onYesPressButton()
{
	m_buttonPressed = true;
	m_buttonPressResult = true;
}

static void onNoPressButton()
{
	m_buttonPressed = true;
	m_buttonPressResult = false;
}

static void createBanner(const char* title)
{
	const char* txt =  title == NULL ? "MsgBox" : title;
    tRectangle rect;
	rect.i16XMin = MSGBOX_UP_LEFT_CORNER_X;
	rect.i16YMin = MSGBOX_UP_LEFT_CORNER_Y;
	rect.i16XMax = MSGBOX_UP_LEFT_CORNER_X+MSGBOX_WIDTH_X;
	rect.i16YMax = MSGBOX_TITLE_HEIGHT_Y+MSGBOX_UP_LEFT_CORNER_Y;
	GrContextForegroundSet(m_drawingCtx, ClrDarkBlue);
	GrRectFill(m_drawingCtx, &rect);
	// Put a Red box around the banner.
	GrContextForegroundSet(m_drawingCtx, ClrRed);
	GrRectDraw(m_drawingCtx, &rect);

	GrContextForegroundSet(m_drawingCtx, ClrYellowGreen);
	GrContextFontSet(m_drawingCtx, &g_sFontCm20);
	GrStringDrawCentered(m_drawingCtx, txt, -1,
						 MSGBOX_UP_LEFT_CORNER_X+(MSGBOX_WIDTH_X/2),
						 MSGBOX_UP_LEFT_CORNER_Y+(MSGBOX_TITLE_HEIGHT_Y/2), 0);
}

static void writeMessage(const char* msg)
{
	const char* txt = msg == NULL ? "InvalidMsg" : msg;
	GrContextForegroundSet(m_drawingCtx, ClrYellowGreen);
	GrContextFontSet(m_drawingCtx, &g_sFontCm14);
	GrStringDrawCentered(m_drawingCtx, txt, -1,
						 MSGBOX_UP_LEFT_CORNER_X+(MSGBOX_WIDTH_X/2),
						 (MSGBOX_UP_LEFT_CORNER_Y+MSGBOX_TITLE_HEIGHT_Y+(MSGBOX_TITLE_HEIGHT_Y/2)), 0);
}

//init
bool uiMessageBoxInit()
{
	m_drawingCtx = uiGetMainDrawingContext();
	return true;
}

bool uiMessageBoxCreate(const char* msgTitle, const char* msg)
{
	m_buttonPressResult = false;
	m_buttonPressed = false;
    WidgetPaint((tWidget *)&m_msgBoxBackground);
    WidgetMessageQueueProcess();
    writeMessage(msg);
    createBanner(msgTitle);
    uint32_t start = uiDelayCounterMsVal();
    while(((uiDelayCounterMsVal() - start) < 5000) && !m_buttonPressed); //5[s]
	return m_buttonPressResult;
}

