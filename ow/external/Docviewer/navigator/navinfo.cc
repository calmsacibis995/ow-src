#ident "@(#)navinfo.cc	1.6 06/11/93 Copyright 1992 Sun Microsystems, Inc."


#include "navinfo.h"
#include <locale.h>

// List of folks who get credit for working on DocViewer.
//
static	char	*creators[] = {
	"The folks at SunSoft, Inc.",
	"Jonathan Aibel",
	"Eric Bergman",
	"Casey Cameron",
	"John Edward",
	"Peter Harootian",
	"Pravin Kumar",
	"Matt Lennon",
	"Eswar Priyadarshan",
	"Bob Yennaco",
	NULL,
};

// Navigator icon.
//
static u_short		icon_bits[] =
{
#include <images/gator.icon>
};

static void	InitMenu(Panel_item menu);


NAVINFO::NAVINFO(Xv_opaque parent)
{
	Xv_opaque	panel;		// popup's panel
	Panel_item	icon_widget;	// Icon that goes on the panel.
	Panel_item	created_by;	// "Created by" menu.
	Panel_item	msg1_widget;
	Panel_item	msg2_widget;
	Panel_item	msg3_widget;
	Server_image	icon;
	STRING		msg_string;
	int		margin_x, margin_y;
	int		icon_x, icon_y;
	int		msg1_x, msg1_y;
	int		msg2_x, msg2_y;
	int		msg3_x, msg3_y;
	int		created_x, created_y;


	assert(parent != NULL);
	DbgFunc("NAVINFO::NAVINFO" << endl);


	margin_x = 10;
	margin_y = 10;


	// Create popup frame; get popup's default panel.
	//
	frame = xv_create(parent, FRAME_CMD,
			FRAME_LABEL, gettext("Navigator: About the Navigator"),
			FRAME_SHOW_RESIZE_CORNER,	FALSE,
			FRAME_SHOW_FOOTER,		FALSE,
			NULL);
	if (frame == NULL)
		OutOfMemory();

	panel = xv_get(frame, FRAME_CMD_PANEL);
	if (panel == NULL)
		OutOfMemory();


	// Create icon widget.
	// Place it in the upper left hand corner.
	//
	icon = xv_create(NULL, SERVER_IMAGE,
			XV_WIDTH,		64,
			XV_HEIGHT,		64,
			SERVER_IMAGE_BITS,	icon_bits,
			XV_NULL);

	icon_x = margin_x;
	icon_y = margin_y;
	icon_widget = xv_create(panel, PANEL_MESSAGE,
			XV_X,				icon_x,
			XV_Y,				icon_y,
			PANEL_LABEL_IMAGE,		icon,
			XV_NULL);


	// Create three message widgets.
	// Place them to the right of the icon.
	//
	msg1_x      = (int) xv_get(icon_widget, XV_WIDTH) + icon_x + 20;
	msg1_y      = margin_y;
	msg_string  = gettext("Navigator Version");
	msg_string += " " + navigator_version;
	msg1_widget = xv_create(panel, PANEL_MESSAGE,
			XV_X,				msg1_x,
			XV_Y,				msg1_y,
			PANEL_LABEL_STRING,		~msg_string,
			PANEL_LABEL_BOLD,		TRUE,
			XV_NULL);

	msg2_x      = msg1_x;
	msg2_y      = (int) xv_get(msg1_widget, XV_HEIGHT) + msg1_y + 10;
	msg_string  = gettext("Copyright 1990-1992 Sun Microsystems, Inc.");
	msg2_widget = xv_create(panel, PANEL_MESSAGE,
			XV_X,				msg2_x,
			XV_Y,				msg2_y,
			PANEL_LABEL_STRING,		~msg_string,
			PANEL_LABEL_BOLD,		FALSE,
			XV_NULL);

	msg3_x      = msg2_x;
	msg3_y      = (int) xv_get(msg2_widget, XV_HEIGHT) + msg2_y + 10;
	msg_string  = gettext("Created ");
	msg_string += creation_date;
	msg3_widget = xv_create(panel, PANEL_MESSAGE,
			XV_X,				msg3_x,
			XV_Y,				msg3_y,
			PANEL_LABEL_STRING,		~msg_string,
			PANEL_LABEL_BOLD,		FALSE,
			XV_NULL);


	// Shrink panel width to fit all the widgets we've created so far.
	// This will allow us to center the "Created by" menu at the bottom
	// of the panel.
	//
	window_fit_width(panel);


	// Create "Created by" abbreviated menu.
	// Center it at the bottom of the panel.
	//
	created_y  = (int) xv_get(msg3_widget, XV_HEIGHT) + msg3_y + 20;
	xv_set(panel, PANEL_LAYOUT, PANEL_HORIZONTAL, XV_NULL);
	created_by = xv_create(panel, PANEL_CHOICE_STACK,
			XV_Y,			created_y,
			PANEL_LAYOUT,		PANEL_HORIZONTAL,
			PANEL_LABEL_STRING,	gettext("Brought to you by:"),
			NULL);

	created_x  = (int) xv_get(panel,      XV_WIDTH) / 2;
	created_x -= (int) xv_get(created_by, XV_WIDTH) / 2;
	xv_set(created_by, XV_X, created_x, XV_NULL);

	InitMenu(created_by);


	// Fit panel and frame around the widgets.
	//
	window_fit(panel);
	window_fit(frame);


	// We're ready to roll...
	//
	objstate.MarkReady();
}

NAVINFO:: ~NAVINFO()                     
{
	DbgFunc("NAVINFO::~NAVINFO" << endl);
}

// Display query history popup.
//
void
NAVINFO::Show()
{
	assert(objstate.IsReady());
	DbgFunc("NAVINFO::Show" << endl);
	EVENTLOG("navigator info");

	xv_set(frame, XV_SHOW, TRUE, XV_NULL);
}

void
InitMenu(Panel_item menu)
{
	int	i;

	DbgFunc("NAVINFO::InitMenu" << endl);

	for (i = 0; creators[i] != NULL; i++) {
		xv_set(menu, PANEL_CHOICE_STRING, i, creators[i], XV_NULL);
	}
}
