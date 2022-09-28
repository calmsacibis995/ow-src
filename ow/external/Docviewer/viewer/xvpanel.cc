#ident "@(#)xvpanel.cc	1.69 11/11/93 Copyright 1989 Sun Microsystems, Inc."

#include "common.h"
#include <stdio.h>	// This is needed for notify.h
#include <math.h>
#include <xview/xview.h>
#include "xvpanel.h"
#include "spothelp.h"

#ifndef	MAX
#define	MAX(a,b)	((a) > (b) ? (a) : (b))
#endif	/* MAX */

#define	INTER_ITEM_SPACE	10

Xv_opaque XVPANEL::_dataKey	= 0;

// Functions called
#ifdef DEBUG
extern void		PrintRect(char *, Xv_opaque);
#endif /* DEBUG */

extern void	NewHandler();

static unsigned short prev_page_bits[] = {
#include "prev_page.icon"
};

static unsigned short next_page_bits[] = {
#include "next_page.icon"
};

XVPANEL::XVPANEL() :
	followLinkItem	(XV_NULL),
	pgBwdButton	(XV_NULL),
	pgFwdButton	(XV_NULL),
	viewButton	(XV_NULL),
	viewMenu	(XV_NULL)
{
	DbgFunc("XVPANEL::XVPANEL: entered" << endl);
}

void
XVPANEL::EventProc(const Xv_window	win,
		   Event *const		event,
		   const Notify_arg	/* unused */)
{
	XVPANEL *const xvp = (XVPANEL *)xv_get(win, XV_KEY_DATA, _dataKey);

	DbgFunc("XVPANEL::EventProc: entered" << endl);

	switch (event_action(event)) {
	case WIN_RESIZE:
			if (xvp) {
				xvp->LayOutPanel();
			}
		break;

	default:
		break;
	}

	return;
}
		   

STATUS
XVPANEL::Init(const Frame frame, const ViewerType vtype, ERRSTK &)
{
	int		hght;
	const int	wdth	= (int) xv_get(frame, XV_WIDTH);

	panel		= (Panel)
		xv_create(frame,		PANEL,
			  PANEL_CLIENT_DATA,	(caddr_t) this,
			  PANEL_LAYOUT,		PANEL_HORIZONTAL,
			  WIN_DESIRED_WIDTH,	WIN_EXTEND_TO_EDGE,
			  XV_X,			0,
			  XV_Y,			0,
			  XV_NULL);

	DbgFunc("XVPANEL::Init: creating panel buttons" << endl);

	if (panel == XV_NULL)
		NewHandler();


	_dataKey = xv_unique_key();

	(void) xv_set(panel,
		      XV_KEY_DATA, _dataKey, (caddr_t) this,
		      WIN_EVENT_PROC,	EventProc,
		      XV_NULL);

	CreateButtons(vtype);

	
	// Calculate the height of the panel from the buttons
	//
	hght = MAX((int) xv_get(printButton, XV_HEIGHT),
		   (int) xv_get(pgBwdButton, XV_HEIGHT));
	hght += 20;

	(void) xv_set(panel,
		      WIN_HEIGHT,	hght,
		      XV_NULL);

	LayOutPanel();

	return(STATUS_OK);
}

void
XVPANEL::CreateButtons(const ViewerType vtype)
{
	const char *followLinkStr	= gettext("Follow Link");
	const char *pageInfoStr		= gettext("Page Info ...");
	const char *standardSizeStr	= gettext("Standard Magnification");
	const char *customMagStr	= gettext("Custom Magnification ...");
	const char *viewStr		= gettext("View   ");
	const char *printButtonStr	= gettext("Print...");
	const char *gobackStr		= gettext("Go Back");
	Server_image	prev_page_glyph;
	Server_image	next_page_glyph;
	Menu_item	mi;

		viewMenu = (Menu) 
		xv_create(XV_NULL,	MENU,
		MENU_CLIENT_DATA,	(caddr_t) this,
		MENU_ITEM,
			MENU_STRING,	followLinkStr,
			MENU_CLIENT_DATA, FOLLOW_LINK,
			MENU_ACTION_PROC, ViewMenuActionProc,
			XV_HELP_DATA,	FOL_LINK_MENUITEM_HELP,
			XV_NULL,
		MENU_ITEM,
			MENU_STRING,	pageInfoStr,
			MENU_CLIENT_DATA, PAGE_INFO,
			MENU_ACTION_PROC, ViewMenuActionProc,
			XV_HELP_DATA,	PAGEINFO_MENUITEM_HELP,
			XV_NULL,
		MENU_ITEM,
			MENU_STRING,	"",
			MENU_INACTIVE,	TRUE,
			XV_NULL,
		MENU_ITEM,
		  	MENU_STRING,	standardSizeStr,
		  	MENU_CLIENT_DATA, STANDARD_MAG,
		  	MENU_ACTION_PROC, ViewMenuActionProc,
		  	XV_HELP_DATA,	STD_SIZE_MENUITEM_HELP,
		  	XV_NULL,
		MENU_ITEM,
			MENU_STRING,	customMagStr,
			MENU_CLIENT_DATA, CUSTOM_MAG,
			MENU_ACTION_PROC, ViewMenuActionProc,
			XV_HELP_DATA,	CUSTOM_MAG_MENUITEM_HELP,
			XV_NULL,

		XV_NULL);

	// PageInfo only exists for DocViewer (and is the default).
	// For HelpViewer, remove it and set the default to be
	// FollowLink.
	//
	mi = (Menu_item) xv_find(viewMenu,	MENUITEM,
			MENU_STRING,	pageInfoStr,
			XV_AUTO_CREATE,	FALSE,
			XV_NULL);

	if (vtype == HELPVIEWER) {
		xv_set(viewMenu, MENU_REMOVE_ITEM, mi, XV_NULL);
		xv_destroy(mi);

		mi = (Menu_item) xv_find(viewMenu, MENUITEM,
			MENU_STRING,	followLinkStr,
			XV_AUTO_CREATE,	FALSE,
			XV_NULL);
	}


	xv_set(viewMenu, MENU_DEFAULT_ITEM, mi, XV_NULL);



	// Create glyphs for Previous/Next Page buttons.
	//
	prev_page_glyph = (Server_image) xv_create(NULL, SERVER_IMAGE,
			XV_WIDTH, 		16,
			XV_HEIGHT, 		16,
			SERVER_IMAGE_BITS, 	prev_page_bits,
			NULL);
	next_page_glyph = (Server_image) xv_create(NULL, SERVER_IMAGE,
			XV_WIDTH,	 	16,
			XV_HEIGHT, 		16,
			SERVER_IMAGE_BITS, 	next_page_bits,
			NULL);

	if (prev_page_glyph == XV_NULL  ||  next_page_glyph == XV_NULL)
		NewHandler();


	viewButton	= (Panel_item)
		xv_create(panel,		PANEL_BUTTON,
			  PANEL_CLIENT_DATA,	(caddr_t) this,
			  PANEL_ITEM_MENU,	viewMenu,
			  PANEL_LABEL_BOLD,	TRUE,
			  PANEL_LABEL_STRING,	viewStr,
			  XV_HELP_DATA,		VIEW_BUTTON_HELP,
			  XV_NULL);

	printButton	= (Panel_item)
		xv_create(panel,		PANEL_BUTTON,
			  PANEL_CLIENT_DATA,	(caddr_t) this,
			  PANEL_LABEL_BOLD,	TRUE,
			  PANEL_LABEL_STRING,	printButtonStr,
			  PANEL_NOTIFY_PROC,	XVPANEL::PrintButtonNotifyProc,
			  XV_HELP_DATA,		PRINT_BUTTON_HELP,
			  XV_SHOW,
				(vtype == HELPVIEWER) ? FALSE : TRUE,
			  XV_NULL);


	pgBwdButton = (Panel_item)
		xv_create(panel,		PANEL_BUTTON,
			  PANEL_CLIENT_DATA,	(caddr_t) this,
			  PANEL_LABEL_IMAGE,	prev_page_glyph,
			  PANEL_NOTIFY_PROC,	XVPANEL::PgButtonNotifyProc,
			  XV_HELP_DATA,		PGBWD_BUTTON_HELP,
			  XV_NULL);

	goBkButton = (Panel_item)
		xv_create(panel,		PANEL_BUTTON,
			  PANEL_CLIENT_DATA,	(caddr_t) this,
			  PANEL_INACTIVE,	TRUE,
			  PANEL_LABEL_BOLD,	TRUE,
			  PANEL_LABEL_STRING,	gobackStr,
			  PANEL_NOTIFY_PROC,	XVPANEL::GoBkButtonNotifyProc,
			  XV_HELP_DATA,		GOBACK_BUTTON_HELP,
			  XV_NULL);

	pgFwdButton = (Panel_item)
		xv_create(panel,		PANEL_BUTTON,
			  PANEL_CLIENT_DATA,	(caddr_t) this,
			  PANEL_LABEL_IMAGE,	next_page_glyph,
			  PANEL_NOTIFY_PROC,	XVPANEL::PgButtonNotifyProc,
			  XV_HELP_DATA,		PGFWD_BUTTON_HELP,
			  XV_NULL);


	if (printButton   == XV_NULL  ||
	    pgBwdButton == XV_NULL  ||
	    pgFwdButton == XV_NULL  ||
	    goBkButton  == XV_NULL)
		NewHandler();
}

void
XVPANEL::EnableFollowLinkItem(BOOL bool)
{
	DbgFunc("XVPANEL::EnableFollowLinkItem: entered" << endl);

	// Find and save the "Follow Link" menu item
	if (!followLinkItem) {
		DbgMed("XVPANEL::EnableFollowLinkItem: " <<
		       "followLinkItem == NULL ... trying to find it" <<
		       endl);

		followLinkItem	= (Menu_item)
			xv_find(viewMenu,		MENUITEM,
				MENU_STRING,		gettext("Follow Link"),
				XV_AUTO_CREATE,		FALSE,
				XV_NULL);
	}

	if (followLinkItem) {
		(void) xv_set(followLinkItem,
			      MENU_INACTIVE,	((bool) ? 0 : 1),
			      XV_NULL);
	}
}

int
XVPANEL::PrintButtonNotifyProc(Panel_item button, Event *)
{
	//Local variables
	XVPANEL	*panel = (XVPANEL *) xv_get(button, PANEL_CLIENT_DATA);

	panel->defaultEventProc(VE__PRINT, (const char *) NULL, panel->cdata);

	return(XV_OK);
}

int
XVPANEL::GoBkButtonNotifyProc(Panel_item button, Event *)
{
	//Local variables
	XVPANEL	*panel = (XVPANEL *) xv_get(button, PANEL_CLIENT_DATA);

	panel->defaultEventProc(VE__GOBACK, (const char *) NULL, panel->cdata);

	return(XV_OK);
}

void
XVPANEL::PgButtonNotifyProc(Panel_item button, Event *)
{
	//Local variables
	XVPANEL	       *panel;

	panel = (XVPANEL *) xv_get(button, PANEL_CLIENT_DATA);

	if (button == panel->pgBwdButton) {	// Prev Page
		panel->defaultEventProc(VE__PREV_PAGE,
					(const char *) NULL,
					panel->cdata);
	}
	else if (button == panel->pgFwdButton) {	// Next Page
		panel->defaultEventProc(VE__NEXT_PAGE,
					(const char *) NULL,
					panel->cdata);
	}

	return;
}

void
XVPANEL::LayOutPanel()
{
	int	panel_width  = (int) xv_get(panel, XV_WIDTH);
	int	panel_height = (int) xv_get(panel, XV_HEIGHT);
	int	button_x, button_y;
	int	button_width;
	int	button_height;


	DbgFunc("XVPANEL::LayOutPanel: entered" << endl);


	// The "previous page", "next page", and "go back" buttons
	// all go together at the right edge of the panel.
	//
	button_width  = (int) xv_get(pgFwdButton, XV_WIDTH);
	button_height = (int) xv_get(pgFwdButton, XV_HEIGHT);
	button_x      = panel_width - button_width - 10;
	button_y      = (panel_height - button_height) / 2;
	(void) xv_set(pgFwdButton, XV_X, button_x, XV_Y, button_y, XV_NULL);

	button_width  = (int) xv_get(pgBwdButton, XV_WIDTH);
	button_height = (int) xv_get(pgBwdButton, XV_HEIGHT);
	button_x      = button_x - button_width - 10;
	button_y      = (panel_height - button_height) / 2;
	(void) xv_set(pgBwdButton, XV_X, button_x, XV_Y, button_y, XV_NULL);

	button_width  = (int) xv_get(goBkButton, XV_WIDTH);
	button_height = (int) xv_get(goBkButton, XV_HEIGHT);
	button_x      = button_x - button_width - 10;
	button_y      = (panel_height - button_height) / 2;
	(void) xv_set(goBkButton, XV_X, button_x, XV_Y, button_y, XV_NULL);


	// The "view" and "magnify" buttons
	// go together at the left edge of the panel.
	//
	button_width  = (int) xv_get(viewButton, XV_WIDTH);
	button_height = (int) xv_get(viewButton, XV_HEIGHT);
	button_x      = 10;
	button_y      = (panel_height - button_height) / 2;
	(void) xv_set(viewButton, XV_X, button_x, XV_Y, button_y, XV_NULL);

	button_width  = (int) xv_get(printButton, XV_WIDTH);
	button_height = (int) xv_get(printButton, XV_HEIGHT);
	button_x      = button_x + (int)xv_get(viewButton, XV_WIDTH) + 10;
	button_y      = (panel_height - button_height) / 2;
	(void) xv_set(printButton, XV_X, button_x, XV_Y, button_y, XV_NULL);


	// Repaint panel and panel widgets.
	//
	panel_paint(panel, PANEL_CLEAR);
}

int
XVPANEL::MinWidth()
{
	int	width = 0;

	// Add up the widths of all the widgets
	width += (int) xv_get(viewButton,  XV_WIDTH);
	width += (int) xv_get(printButton,   XV_WIDTH);
	width += (int) xv_get(pgBwdButton, XV_WIDTH);
	width += (int) xv_get(goBkButton,  XV_WIDTH);
	width += (int) xv_get(pgFwdButton, XV_WIDTH);

	// Add the widths of all the spaces
	width += 20;	// spaces at either end
	width += 40;	// spaces between buttons

	return(width);
}

void
XVPANEL::SetWidth(int wdth)
{
	DbgFunc("XVPANEL::SetWidth: wdth:\t" << wdth << endl);

	if (wdth > 0) {
		(void) xv_set(panel, XV_WIDTH, wdth, XV_NULL);
		LayOutPanel();
	}

	return;
}

void
XVPANEL::ViewMenuActionProc(Menu menu, Menu_item menuItem)
{
	//Local variables
	XVPANEL		       *panel;
	ViewMenuValue		menuValue;
	
	DbgFunc("XVPANEL::ViewMenuActionProc: entered\n");

	panel = (XVPANEL *) xv_get(menu, MENU_CLIENT_DATA);

	menuValue = (ViewMenuValue) xv_get(menuItem, MENU_CLIENT_DATA);

	switch (menuValue) {

	case FOLLOW_LINK:	// Follow selected link
		panel->defaultEventProc(VE__FOLLOW_LINK,
					(const char *) NULL,
					panel->cdata);
		panel->EnableFollowLinkItem(BOOL_FALSE);
		break;

	case PAGE_INFO:		// PageInfo
		panel->defaultEventProc(VE__PAGE_INFO,
					(const char *) NULL,
					panel->cdata);
		break;

	case STANDARD_MAG:	// Standard Size
		panel->defaultEventProc(VE__STD_SIZE,
					(const char *) NULL,
					panel->cdata);
		break;

	case CUSTOM_MAG:	// Allow user to custom magnify
		panel->defaultEventProc(VE__CUSTOM_MAG,
					(const char *) NULL,
					panel->cdata);
		break;

	default:
		assert(0);
		DbgHigh("XVPANEL::ViewMenuActionProc: \"" << menuValue
			<< "\" unknown menu value" << endl);
		break;
	}
	return;
}

XVPANEL::~XVPANEL()
{
	DbgFunc("XVPANEL::~XVPANEL entered" << endl);
}
