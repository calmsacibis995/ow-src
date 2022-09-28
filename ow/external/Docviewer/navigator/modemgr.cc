#ident "@(#)modemgr.cc	1.26 93/12/20 Copyright 1990-1992 Sun Microsystems, Inc."

#include "modemgr.h"
#include <doc/notify.h>
#include <xview/notify.h>


// Declare static members of class MODEMGR.
//
BOOL		MODEMGR::main_is_inited		= BOOL_FALSE;
Xv_opaque	MODEMGR::main_panel		= XV_NULL;
Xv_opaque	MODEMGR::main_view_button	= XV_NULL;
Xv_opaque	MODEMGR::main_bookmark_button	= XV_NULL;
Xv_opaque	MODEMGR::main_browse_button	= XV_NULL;
Xv_opaque	MODEMGR::main_mode_choice	= XV_NULL;
#ifdef	USER_COMMENTS
Xv_opaque	MODEMGR::main_comments_button	= XV_NULL;
#endif	USER_COMMENTS

static int	OBJ_KEY = (int) xv_unique_key();

// Navigator frame dimensions in characters
// (i.e., these values get multiplied by "panel font point size").
//
static const int	DEFAULT_FRAME_WIDTH (40);
static const int	DEFAULT_FRAME_HEIGHT(40);


MODEMGR::MODEMGR(Xv_opaque frame, NOTIFY *ntfy) :
	notify	(ntfy)
{
	int		fontsize;


	assert(frame != NULL);
	assert(notify != NULL);
	DbgFunc("MODEMGR::MODEMGR" << endl);


	// Initialize Main Widgets if necessary.
	// (a one-time-only proposition).
	//
	InitMainWidgets(frame);


	// Create panel upon which all widgets for this mode are placed.
	//
	mode_panel = xv_create(frame, PANEL,
			XV_X,			0,
			WIN_BELOW,		main_panel,
			WIN_DESIRED_WIDTH,	WIN_EXTEND_TO_EDGE,
			WIN_DESIRED_HEIGHT,	WIN_EXTEND_TO_EDGE,
			WIN_CLIENT_DATA,	(caddr_t) this,
			XV_SHOW,		FALSE,
			NULL);
	if (mode_panel == XV_NULL)
		OutOfMemory();

	notify_interpose_event_func(	mode_panel, 
					(Notify_func) MODEMGR::PanelEvent, 
					NOTIFY_IMMEDIATE);


	// Set the minimum dimensions for this window via the window manager.
	// XXX	Here we're making some assumptions
	//	about the size of each mode panel that may not hold
	//	if the panel layout or widget labels change.
	//	This should be done dynamically for each mode panel
	//	but I don't feel like doing that today.
	//
#define	MIN_FRAME_WIDTH		35	// min width in character units
#define	MIN_FRAME_HEIGHT	30	// min height in character units

	fontsize = (int) xv_get(xv_get(mode_panel, XV_FONT), FONT_SIZE);

	xv_set (frame, FRAME_MIN_SIZE, MIN_FRAME_WIDTH * fontsize,
				MIN_FRAME_HEIGHT * fontsize, NULL);

	// Ready to roll...
	//
	objstate.MarkReady();
}

MODEMGR::~MODEMGR()
{
	DbgFunc("MODEMGR::MODEMGR" << endl);
}

void
MODEMGR::InitMainWidgets(Xv_opaque frame)
{
	int	button_x, button_y;
	int	button_width;
	int	button_height;
	int	mode_x, mode_y;
	int	fontsize;


	DbgFunc("MODEMGR::InitMainWidgets" << endl);
	if (main_is_inited)
		return;


 	// Create widget panel.
	//
	main_panel = xv_create(frame, PANEL,
			XV_X,			0,
			XV_Y,			0,
			WIN_DESIRED_WIDTH,	WIN_EXTEND_TO_EDGE,
			WIN_DESIRED_HEIGHT,	WIN_EXTEND_TO_EDGE,
			XV_KEY_DATA,		OBJ_KEY,   (caddr_t)this,
			NULL);

	if (main_panel == NULL)
		OutOfMemory();


	// Set default window frame dimensions.
	// Note that we do this here *before* creating any panel widgets
	// to avoid the situation where the panel's automatic widget layout
	// algorithm does goofy things to make widgets fit in the window.
	//
	// XXX	We're just guessing here as to the correct dimensions.
	//	Dimensions *should* be based on the needs of the individual
	//	Mode Panels, but that's too hard to do.
	//
	fontsize = (int) xv_get(xv_get(main_panel, XV_FONT), FONT_SIZE);
	if (fontsize <= 0)	//XXX happens sometimes - don't know why
		fontsize = 12;
	xv_set(frame,
			XV_WIDTH,	DEFAULT_FRAME_WIDTH  * fontsize,
			XV_HEIGHT,	DEFAULT_FRAME_HEIGHT * fontsize,
			NULL);


	// Create panel buttons: View, Modify Library, New Bookmark.
	//
	button_x = 10;
	button_y = 10;
	main_view_button = xv_create(main_panel, PANEL_BUTTON,
			XV_X,			button_x,
			XV_Y,			button_y,
			PANEL_LABEL_STRING,	gettext("View"),
			PANEL_NOTIFY_PROC,	MODEMGR::WidgetEvent,
			PANEL_CLIENT_DATA,	MM_VIEW_EVENT,
			XV_HELP_DATA,		VIEW_BUTTON_HELP,
			NULL);
	button_width  = (int) xv_get(main_view_button, XV_WIDTH);
	button_height = (int) xv_get(main_view_button, XV_HEIGHT);

	button_x += button_width + 10;
	main_browse_button = xv_create(main_panel, PANEL_BUTTON,
			XV_X,			button_x,
			XV_Y,			button_y,
			PANEL_LABEL_STRING,	gettext("Modify Library..."),
			PANEL_CLIENT_DATA,	(caddr_t)this,
			PANEL_NOTIFY_PROC,	MODEMGR::WidgetEvent,
			PANEL_CLIENT_DATA,	MM_BROWSE_EVENT,
			XV_HELP_DATA,		BROWSE_BUTTON_HELP,
			NULL);
	button_width  = (int) xv_get(main_browse_button, XV_WIDTH);

	button_x += button_width + 10;
	main_bookmark_button = xv_create(main_panel, PANEL_BUTTON,
			XV_X,			button_x,
			XV_Y,			button_y,
			PANEL_LABEL_STRING,	gettext("New Bookmark..."),
			PANEL_NOTIFY_PROC,	MODEMGR::WidgetEvent,
			PANEL_CLIENT_DATA,	MM_BOOKMARK_EVENT,
			XV_HELP_DATA,		NEW_BOOKMARK_BUTTON_HELP,
			NULL);
	button_width  = (int) xv_get(main_bookmark_button, XV_WIDTH);

#ifdef	USER_COMMENTS
	button_x += button_width + 20;
	main_comments_button = xv_create(main_panel, PANEL_BUTTON,
			XV_X,			button_x,
			XV_Y,			button_y,
			PANEL_LABEL_STRING,	gettext("Comments..."),
			PANEL_NOTIFY_PROC,	MODEMGR::WidgetEvent,
			PANEL_CLIENT_DATA,	MM_COMMENTS_EVENT,
//XXX			XV_HELP_DATA,		COMMENTS_BUTTON_HELP,
			NULL);
#endif	USER_COMMENTS


	// once the mode choice is created add the choice strings,
	//
	mode_x = 10;
	mode_y = button_y + button_height + 10;
	main_mode_choice = xv_create(main_panel, PANEL_CHOICE,
			XV_X,			mode_x,
			XV_Y,			mode_y,
			PANEL_CHOICE_STRINGS,		
				gettext("Contents"),
				gettext("Search"),
				gettext("Bookmarks"),
				NULL,
			PANEL_NOTIFY_PROC,	MODEMGR::WidgetEvent,
			PANEL_CLIENT_DATA,	MM_MODE_SELECT_EVENT,
			NULL);

	xv_set(main_panel, PANEL_LAYOUT, PANEL_HORIZONTAL, NULL);


	// Make sure we've got everything we need.
	//
	if (main_view_button     == NULL ||
	    main_browse_button   == NULL ||
	    main_bookmark_button == NULL ||
	    main_mode_choice     == NULL) {
		OutOfMemory();
	}


	window_fit_height(main_panel);
	main_is_inited = BOOL_TRUE;
}

void
MODEMGR::SetMode(int mode)
{
	char	*panel_help;
	char	*button_help;
	int	modeval;

	assert(objstate.IsReady());
	DbgFunc("MODEMGR::SetMode: " << mode << endl);


	switch (mode) {

	case MODE_TOC:
		modeval = 0;
		panel_help  = MODE_TOC_HELP1;
		button_help = MODE_BUTTON_TOC_HELP;
		break;

	case MODE_SEARCH:
		modeval = 1;
		panel_help  = MODE_SEARCH_HELP1;
		button_help = MODE_BUTTON_SEARCH_HELP;
		break;

	case MODE_BOOKMARK:
		modeval = 2;
		panel_help  = MODE_BOOKMARK_HELP1;
		button_help = MODE_BUTTON_BOOKMARK_HELP;
		break;

	case MODE_NONE:
	default:
		assert(0);
		break;
	}

	xv_set(main_mode_choice,
		PANEL_VALUE,	modeval,
		XV_HELP_DATA,	button_help,
		NULL);

	xv_set(main_panel,
		XV_HELP_DATA,	panel_help,
		XV_KEY_DATA,	OBJ_KEY,   (caddr_t) this,
		NULL);
}

int
MODEMGR::GetMode()
{
	assert(objstate.IsReady());
	return((int) xv_get(main_mode_choice, PANEL_VALUE));
}

void
MODEMGR::WidgetEvent(Panel_item widget, Event *)
{
	Panel	panel;
	MODEMGR	*modemgr;
	int	event;


	panel = (Panel) xv_get(widget, XV_OWNER);
	assert(panel != NULL);
	modemgr = (MODEMGR *) xv_get(panel, XV_KEY_DATA, OBJ_KEY);
	assert(modemgr != NULL);
	event = (int) xv_get(widget, PANEL_CLIENT_DATA);

	DbgFunc("MODEMGR::WidgetEvent: " << (int)event << endl);

	modemgr->EventHandler(event, (caddr_t)widget);
}

void
MODEMGR::MenuEvent(Menu menu, Menu_item item)
{
	MODEMGR	*modemgr;
	int	event;


	modemgr = (MODEMGR *) xv_get(menu, XV_KEY_DATA, OBJ_KEY);
	event   = (int)  xv_get(item, MENU_CLIENT_DATA);

	assert(modemgr != NULL);
	DbgFunc("MODEMGR::MenuEvent: " << (int)event << endl);

	modemgr->EventHandler(event, (caddr_t)menu);
}

Notify_value
MODEMGR::PanelEvent(	Panel panel,
			Event *event,
			Notify_arg arg, 
			Notify_event_type type)
{
	MODEMGR	*modemgr;
	

	if (event_action(event) != WIN_RESIZE) {
		return(notify_next_event_func(panel, (Notify_event)event, 
						arg, type));
	}


	DbgFunc("PanelEvent: resize event\n");
	EVENTLOG("resize navigator window");

	modemgr = (MODEMGR *)xv_get(panel, WIN_CLIENT_DATA);
	assert(modemgr != NULL);
		
	modemgr->EventHandler(MM_PANEL_RESIZE_EVENT, (caddr_t)panel);

	return(NOTIFY_DONE);
}

void
MODEMGR::DefaultEventHandler(int event, caddr_t /*event_obj*/)
{
	assert(objstate.IsReady());
	DbgFunc("MODEMGR::DefaultEventHandler: " << event << endl);

	// Just pass this event on to the registered event handler.
	//
	if (event_handler != NULL)
		(*event_handler)(event, (caddr_t)this, event_client_data);
}
