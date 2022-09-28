#ident "@(#)xvcanvas.cc	1.142 05/23/94 Copyright 1992 Sun Microsystems, Inc."

#include "common.h"
#include "xvcanvas.h"
#include "viewer.h"
#include <xview/canvas.h>
#include <xview/panel.h>
#include "dpsdebug.h"

typedef enum {
	NEXT_PAGE	= 1,
	PREV_PAGE	= 2,
	REDISPLAY	= 3,
	GO_BACK		= 4
} CanMenuValue;

Xv_opaque XVCANVAS::dataKey = 0;
Xv_opaque XVCANVAS::canvasKey = 0;

XVCANVAS::XVCANVAS() :
	canvasMenu	(XV_NULL),
	cdata		(NULL),
	defaultEventProc(NULL),
	sbar		(NULL)
{
	DbgFunc("XVCANVAS::XVCANVAS: entered" << endl);
}

STATUS
XVCANVAS::CreateMenu(const Frame frame, ERRSTK &err)
{
	STATUS status = STATUS_OK;

	DbgFunc("XVCANVAS::CreateCanvasMenu: entered" << endl);

	canvasKey = xv_unique_key();

	canvasMenu = (Menu) xv_create(XV_NULL,	MENU,
			XV_KEY_DATA,		canvasKey, (void *) this,
			MENU_GEN_PIN_WINDOW,	frame, gettext("Viewer"),
			MENU_ITEM,
			MENU_STRING,		gettext("Next Page"),
			MENU_VALUE,		NEXT_PAGE,
			MENU_ACTION_PROC,	XVCANVAS::MenuActionProc,
			XV_HELP_DATA,		NEXTPAGE_MENUITEM_HELP,
			XV_NULL,
			MENU_ITEM,
			MENU_STRING,		gettext("Previous Page"),
			MENU_VALUE,		PREV_PAGE,
			MENU_ACTION_PROC,	XVCANVAS::MenuActionProc,
			XV_HELP_DATA,		PREVPAGE_MENUITEM_HELP,
			XV_NULL,
			MENU_ITEM,
			MENU_STRING,		gettext("Redisplay"),
			MENU_VALUE,		REDISPLAY,
			MENU_ACTION_PROC,	XVCANVAS::MenuActionProc,
			XV_HELP_DATA,		REDISPLAY_MENUITEM_HELP,
			XV_NULL,
			MENU_ITEM,
			MENU_STRING,		gettext("Go Back"),
			MENU_VALUE,		GO_BACK,
			MENU_ACTION_PROC,	XVCANVAS::MenuActionProc,
			XV_HELP_DATA,		GOBACK_MENUITEM_HELP,
			XV_NULL,
			MENU_CLIENT_DATA,	(caddr_t) this,
			MENU_DEFAULT,		NEXT_PAGE,
			XV_VISUAL_CLASS,	PseudoColor,
			NULL);

	if (!canvasMenu) {
		DbgHigh("XVCANVAS::CreateMenu: couldn't create menu" << endl);
		err.Init(gettext("Could not create menu on canvas"));
		status = STATUS_FAILED;
	}

	return(status);
}


// Low-level canvas input event handler.
void
XVCANVAS::EventProc(const Xv_opaque can, Event *const event)
{
        DPSDbgFunc("Entered XVCANVAS::EventProc");
	XVCANVAS *const	canObj	=
		(XVCANVAS *) xv_get(can, XV_KEY_DATA, XVCANVAS::dataKey);
	VIEWER* viewer = (VIEWER *) canObj->cdata;

#ifdef	DEBUG
	const int	xloc	= event_x(event);
	const int	yloc	= event_y(event);
#endif

	DbgFunc("XVCANVAS::EventProc:" << endl);
	assert(canObj);
	assert(canObj->objstate.IsReady());

	if (event_is_up(event))
		return;

	DbgLow("Received XView Event:" << endl << event);

	switch (event_action(event)) {

	case ACTION_GO_PAGE_FORWARD:
		DbgLow("XVCANVAS:EventHandler: "
		       << "ACTION_GO_PAGE_FORWARD at:\t"
		       << "(" << xloc << ", " << yloc << ")" << endl);
		DPSDbgFunc("XVCANVAS::EventProc. ACTION_GO_PAGE_FORWARD");
		DPSDbgFunc("XVCANVAS::EventProc. Invoking canvas defaultEventProc");
		canObj->defaultEventProc(VE__NEXT_PAGE,
					 (const void *) NULL,
					 (const void *) canObj->cdata);
		
		break;
		
	case ACTION_GO_DOCUMENT_START:
	case ACTION_GO_LINE_BACKWARD:
		DbgLow("XVCANVAS:EventHandler: "
		       << "ACTION_GO_DOCUMENT_START at:\t"
		       << "(" << xloc << ", " << yloc << ")" << endl);
		DPSDbgFunc("XVCANVAS::EventProc. ACTION_GO_DOCUMENT_START");
		DPSDbgFunc("XVCANVAS::EventProc. Invoking canvas defaultEventProc");
		canObj->defaultEventProc(VE__PREV_DOC, (const void *) NULL,
					 (const void *) canObj->cdata);
		break;
		
	case ACTION_GO_PAGE_BACKWARD:
		DbgLow("XVCANVAS:EventHandler: "
		       << "ACTION_GO_PAGE_BACKWARD at:\t"
		       << "(" << xloc << ", " << yloc << ")" << endl);
		DPSDbgFunc("XVCANVAS::EventProc. ACTION_GO_PAGE_BACKWARD");
		DPSDbgFunc("XVCANVAS::EventProc. Invoking canvas defaultEventProc");
		canObj->defaultEventProc(VE__PREV_PAGE,
					 (const void *) NULL,
					 (const void *) canObj->cdata);
		break;

	case ACTION_GO_DOCUMENT_END:
	case ACTION_GO_LINE_END:
		DbgLow("XVCANVAS:EventHandler: "
		       << "ACTION_GO_DOCUMENT_END at:\t"
		       << "(" << xloc << ", " << yloc << ")" << endl);
		DPSDbgFunc("XVCANVAS::EventProc. ACTION_GO_DOCUMENT_END");
		DPSDbgFunc("XVCANVAS::EventProc. Invoking canvas defaultEventProc");
		canObj->defaultEventProc(VE__NEXT_DOC,
					 (const void *) NULL,
					 (const void *) canObj->cdata);
		break;

	case ACTION_SELECT:
		DbgLow("XVCANVAS:EventHandler: "
		       << "ACTION_SELECT at:\t"
		       << "(" << xloc << ", " << yloc << ")" << endl);
		DPSDbgFunc("XVCANVAS::EventProc. ACTION_SELECT");
		DPSDbgFunc("XVCANVAS::EventProc. Invoking canvas defaultEventProc");
		canObj->defaultEventProc(VE__SELECT,
					 (const void *) event,
					 (const void *) canObj->cdata);
		break;

		
	case ACTION_UNDO:
		DbgLow("XVCANVAS:EventHandler: ACTION_UNDO at:\t"
		       << "(" << xloc << ", " << yloc << ")" << endl);
		DPSDbgFunc("XVCANVAS::EventProc. ACTION_UNDO");
		DPSDbgFunc("XVCANVAS::EventProc. Invoking canvas defaultEventProc");
		canObj->defaultEventProc(VE__GOBACK,
					 (const void *) NULL,
					 (const void *) canObj->cdata);
		break;

	default:
		DPSDbgFunc("XVCANVAS::EventProc. Default event");
		break;
	}

        DPSDbgFunc("Leaving XVCANVAS::EventProc");
	return;
}

STATUS
XVCANVAS::Handle4BitVisual(Display *const	dpy,
			   int			*visualClass,
			   int			*depth,
			   ERRSTK		&err)
{
	int		nvisuals;
	XVisualInfo	*vList;

	int		depths[3];
	STATUS		status	= STATUS_OK;

	const int	screen = DefaultScreen(dpy);

	volatile int	cntr;

	DbgFunc("XVCANVAS::Handle4BitVisual:" << endl);

	// The priority order of visuals is 8, 24, 1 --- 24-bit images
	// are too cumbersome; 1-bit images don't have color; 8-bit
	// images are a nice medium
	//
	depths[0] = 8;
	depths[1] = 24;
	depths[2] = 1;

	for (cntr = 0; cntr < 3; cntr++) {
		vList = VisualInfo(dpy, screen, depths[cntr], &nvisuals);

		if (vList && nvisuals > 0) {
			break;
		}
	}

	if (cntr < 3) {
		*depth = depths[cntr];
	}

	switch (cntr) {
	case 0:	// 8-bit visual
		if ((*visualClass == GrayScale) ||
		    (*visualClass == StaticGray)) {
			*visualClass = StaticGray;
		}
		else {
			*visualClass = StaticColor;
		}
		break;

	case 1:	// 24-bit visual
		*visualClass = TrueColor;
		break;

	case 2:	// 1-bit visual
		*visualClass = StaticGray;
		break;

	default:
		*depth = 4;

		// Only 4 bit visuals... beeg trouble!
		//
		err.Init(gettext("Only 4-bit visuals available"));
		status = STATUS_FAILED;
		break;
	}

	if (vList) {
		(void) XFree((char *) vList);
	}

	return(status);
}

// Low-level canvas menu action proc
void
XVCANVAS::MenuActionProc(Menu menu, Menu_item menuItem)
{

	// Local variables
	XVCANVAS *const		canObj	=
		(XVCANVAS *) xv_get(menu, XV_KEY_DATA, canvasKey);

	const CanMenuValue	menuValue =
		(CanMenuValue) xv_get(menuItem, MENU_VALUE);

	DbgFunc("XVCANVAS::MenuActionProc: entered" << endl);

	assert((int) menuValue >= NEXT_PAGE && (int) menuValue <= GO_BACK);
	assert(canObj);
	assert(canObj->objstate.IsReady());

	switch (menuValue) {

	case NEXT_PAGE:
		canObj->defaultEventProc(VE__NEXT_PAGE,
					 (const void *) NULL,
					 (const void *) canObj->cdata);
		
		break;

	case PREV_PAGE:
		canObj->defaultEventProc(VE__PREV_PAGE,
					 (const void *) NULL,
					 (const void *) canObj->cdata);
		
		break;

	case REDISPLAY:
		canObj->defaultEventProc(VE__REDISPLAY,
					 (const void *) NULL,
					 (const void *) canObj->cdata);
		break;

	case GO_BACK:
		canObj->defaultEventProc(VE__GOBACK,
					 (const void *) NULL,
					 (const void *) canObj->cdata);
		break;
	}

	return;
}


STATUS
XVCANVAS::Setup(const Frame	frame,
		const Xv_window	win,
		ERRSTK		&err)
{
	STATUS		status	= STATUS_OK;

	DbgFunc("XVCANVAS::Init: entered" << endl);

	assert(frame && win);
	assert(objstate.IsGettingReady());

	dataKey = xv_unique_key();

	(void)	   xv_set(win,
			  WIN_CONSUME_EVENTS,
			  ACTION_ADJUST,
			  ACTION_GO_DOCUMENT_END,
			  ACTION_GO_DOCUMENT_START,
			  ACTION_GO_LINE_END,
			  ACTION_GO_LINE_BACKWARD,
			  ACTION_GO_PAGE_BACKWARD,
			  ACTION_GO_PAGE_FORWARD,
			  ACTION_MENU,
			  ACTION_SELECT,
			  ACTION_UNDO,
			  WIN_MOUSE_BUTTONS,
			  WIN_RIGHT_KEYS,
			  XV_NULL,
			  WIN_IGNORE_EVENTS,
			  LOC_WINENTER,
			  LOC_WINEXIT,
			  WIN_LEFT_KEYS,
			  WIN_ASCII_EVENTS,
			  WIN_TOP_KEYS,
			  WIN_META_EVENTS,
			  XV_NULL,
			  XV_NULL);

	(void) xv_set(win,
		      XV_KEY_DATA, dataKey, (void *) this,
		      XV_HELP_DATA, VIEWER_CANVAS_HELP,
		      XV_NULL);

	status = CreateMenu(frame, err);

	if (status != STATUS_OK)
		return (status);

	sbar = new XVSCROLLBAR(CanvasHandle());
	status = sbar->Init();
	if (status != STATUS_OK)
		console.Message(gettext("can't initialize scroll bar"));

	return(status);
}


// Return a reasonable visual class for this
//
STATUS
XVCANVAS::VisualClass(const Frame	frame,
		      int		*visualClass,
		      int		*depth,
		      ERRSTK		&err)
{
	STATUS		status = STATUS_OK;

	Display *const	dpy	= (Display *) xv_get(frame, XV_DISPLAY);

	DbgFunc("XVCANVAS::VisualClass:" << endl);

	*depth		= (int) xv_get(frame, XV_DEPTH);
	*visualClass	= (int) xv_get(frame, XV_VISUAL_CLASS);

	/*
	 * Check the depth... Then, check the default visual, and set our
	 * canvas' visual based on that info. For now, depth = 4 is a bit of
	 * a problem. Can't define a pixmap or canvas of that depth, so check
	 * to see if we can create one of depth 8 (so color will look OK),
	 * and if not, try depth 24, and if can't get that, then give them a
	 * canvas of depth 1 and they'll have to live with it.
	 */

	switch (*depth) {

	case 1:
		*visualClass = StaticGray;
		break;

	case 4:
		status = Handle4BitVisual(dpy, visualClass, depth, err);

		if (status != STATUS_OK) {
			err.Push(gettext("Cannot create canvas with 4-bit visual"));
		}

		break;

	case 8:
		if (*visualClass == GrayScale || *visualClass == StaticGray) {
			*visualClass = StaticGray;
		}
		else {
			*visualClass = StaticColor;
		}

		break;

	case 24:
		*visualClass = TrueColor;
		break;

	default:
		*visualClass = StaticColor;
		notify->Warning(gettext("Unknown depth \"%d\" - using StaticColor visual"),
				depth);
		break;
	}

	return(status);
}

XVisualInfo *
XVCANVAS::VisualInfo(Display *const	dpy,
		     const int		screen,
		     const int		depth,
		     int		*nvisuals)
{
	XVisualInfo	vTemplate;
	const u_long	vmask	= (VisualScreenMask | VisualDepthMask);

	vTemplate.screen	= screen;
	vTemplate.depth		= depth;

	return(XGetVisualInfo(dpy, vmask, &vTemplate, nvisuals));
}

XVCANVAS::~XVCANVAS()
{
	DbgFunc("XVCANVAS::~XVCANVAS entered" << endl);

	XVCANVAS::dataKey = 0;
}

#ifdef	DEBUG
ostream &
operator <<(ostream &ostr, Event const *event)
{
	(ostr << "\tevent->ie_code\t= " << event->ie_code << endl <<
	 "\tevent->action\t= " << event_action(event) << endl <<
	 "\tevent->ie_shiftmask\t= " << event_shiftmask(event) << endl <<
	 "\tevent->ie_locx\t= " << event_x(event) << endl <<
	 "\tevent->ie_locy\t= " << event_y(event) << endl <<
	 "\tevent->ie_time\t= " << event_time(event).tv_sec << " " <<
	 event_time(event).tv_usec << endl);

	return(ostr);
}
#endif
