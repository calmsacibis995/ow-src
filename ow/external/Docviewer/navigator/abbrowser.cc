#ident "@(#)abbrowser.cc	1.19 93/12/20 Copyright 1992 Sun Microsystems, Inc."

#include "abbrowser.h"
#include "winlist.h"
#include <doc/abinfo.h>
#include <doc/abgroup.h>
#include <doc/abclient.h>
#include <doc/bookshelf.h>
#include <doc/notify.h>
#include <xview/notify.h>


// Glyph bits.
//
static u_short checkmark_bits[16] = {
#include <images/inverted.check.pr>
};

static u_short blank_bits[16] = {
#include <images/blank.pr>
};

extern int	SortEntriesByTitle(const void *, const void *);

Xv_opaque	ABBROWSER::checkmark_glyph = XV_NULL;
Xv_opaque	ABBROWSER::blank_glyph     = XV_NULL;
Xv_opaque	ABBROWSER::label_text      = XV_NULL;
Xv_opaque	ABBROWSER::label_glyph     = XV_NULL;


// Browser constructor.
//
ABBROWSER::ABBROWSER(Xv_opaque parent, ABGROUP &abgroup_arg) :
	abgroup			(abgroup_arg),
	event_handler		(NULL),
	event_client_data	(NULL)
{
	int		x, y;			// widget locations
	int		label_x, label_y;
	int		label_height;
	int		min_width, min_height;	// minimum frame dimensions
	STRING		label;


	assert( ! objstate.IsReady());
	DbgFunc("ABBROWSER::ABBROWSER" << endl);
	objstate.MarkGettingReady();


	// Create frame for Browser popup window.
	// XXX - width and height are kinda arbitrary,
	// XXX and don't scale with font size, etc.
	//
	frame = xv_create(parent, FRAME_CMD,
			FRAME_SHOW_RESIZE_CORNER,	TRUE,
			XV_WIDTH,			400,
			XV_HEIGHT,			300,
			XV_SHOW,			FALSE,
			WIN_USE_IM,			FALSE,
		        XV_NULL);

	if (frame == NULL)
		OutOfMemory();


	notify = new NOTIFY(frame);
	notify->Title(gettext("AnswerBook Navigator: Modify Library"));


	// Get pop-up's panel.
	//
	if ((panel = xv_get(frame, FRAME_CMD_PANEL))  ==  NULL)
		OutOfMemory();

	xv_set(panel,
		PANEL_LAYOUT,		PANEL_VERTICAL,
		WIN_DESIRED_WIDTH,	WIN_EXTEND_TO_EDGE,
		WIN_DESIRED_HEIGHT,	WIN_EXTEND_TO_EDGE,
		WIN_CLIENT_DATA,	(caddr_t) this,
		XV_HELP_DATA,		BROWSER_HELP,
		XV_NULL);

	notify_interpose_event_func(	panel,
					(Notify_func)ABBROWSER::PanelEvent, 
					NOTIFY_IMMEDIATE);


	// Create checkmark glyph, blank glyph for use in AnswerBook list.
	//
	checkmark_glyph = xv_create(NULL, SERVER_IMAGE,
		XV_WIDTH,		16,
		XV_HEIGHT,		16,
		SERVER_IMAGE_BITS,	checkmark_bits,
		XV_NULL);
	blank_glyph = xv_create(NULL, SERVER_IMAGE,
		XV_WIDTH,		16,
		XV_HEIGHT,		16,
		SERVER_IMAGE_BITS,	blank_bits,
		XV_NULL);

	if (checkmark_glyph == NULL  ||  blank_glyph == NULL)
		OutOfMemory();


	// Create label for Bookshelf list:
	//	"AnswerBooks to include in Library..."
	//
	label_x = 10;
	label_y = 10;
	label   = gettext("Select AnswerBooks to include in Library:"),
	label_text = xv_create(panel, PANEL_MESSAGE,
		XV_X,			label_x,
		XV_Y,			label_y,
		PANEL_LABEL_STRING,	~label,
		PANEL_LABEL_BOLD,	TRUE,
		XV_NULL);

	label_height = (int) xv_get(label_text, XV_HEIGHT);


	// Create list object for displaying/selecting AnswerBooks.
	//
	x = label_x;
	y = label_y + label_height + 5;
	winlist = new WINLIST(panel, x, y);
	xv_set(winlist->XvHandle(),
		PANEL_CHOOSE_NONE,	TRUE,
		PANEL_CHOOSE_ONE,	FALSE,
		PANEL_LIST_DISPLAY_ROWS, 8,
		PANEL_LAYOUT,		PANEL_VERTICAL,
		XV_HELP_DATA,		BROWSER_LIST_HELP,
		XV_NULL);

	// We want to 'interpose' on panel list selection events
	// in order to put check marks beside items as they are selected.
	//
	winlist->SetEventHandler(ABBROWSER::DispatchEvent, (caddr_t)this);



	// Create "Apply" and "Reset" buttons.
	//
	apply_button = xv_create(panel, PANEL_BUTTON,
		PANEL_LABEL_STRING,	gettext("Apply"),
		PANEL_CLIENT_DATA,	(caddr_t) BROWSER_APPLY_EVENT,
		PANEL_NOTIFY_PROC,	ABBROWSER::ButtonEvent,
		XV_HELP_DATA,		BROWSER_APPLY_BUTTON_HELP,
		XV_NULL);

	reset_button = xv_create(panel, PANEL_BUTTON,
		PANEL_LABEL_STRING,	gettext("Reset"),
		PANEL_CLIENT_DATA,	(caddr_t) BROWSER_RESET_EVENT,
		PANEL_NOTIFY_PROC,	ABBROWSER::ButtonEvent,
		XV_HELP_DATA,		BROWSER_RESET_BUTTON_HELP,
		XV_NULL);

	if (apply_button == NULL  ||  reset_button == NULL)
		OutOfMemory();


	// Set minimum size of Browser's frame.
	//
	min_width   = (int) xv_get(apply_button, XV_WIDTH);
	min_width  += (int) xv_get(reset_button, XV_WIDTH);
	min_width  *= 2;

	// Figure out min size of panel list
	min_height  = ((int) xv_get (winlist->XvHandle (), XV_HEIGHT) - 
		       ((int) xv_get(winlist->XvHandle(), PANEL_LIST_ROW_HEIGHT)
			 * 8));
	min_height += ((int) xv_get (winlist->XvHandle(), PANEL_LIST_ROW_HEIGHT)
				* 3);
	min_height += (int) xv_get (winlist->XvHandle(), XV_Y);
	min_height += (int) xv_get (apply_button, XV_HEIGHT);
	min_height += 20;

	xv_set (frame, FRAME_MIN_SIZE, min_width, min_height, NULL);

	// Adjust all the widgets so they'll fit in the Browser window.
	//
	Resize();


	// We're ready to roll...
	//
	objstate.MarkReady();
}

// Browser destructor.
//
ABBROWSER::~ABBROWSER()
{
	DbgFunc("ABBROWSER::~ABBROWSER" << endl);

	delete winlist;
	delete notify;
}

// Highlight the Browser entries that are in the current ABGROUP.
//
void
ABBROWSER::Update()
{
	LISTX<ABINFO*>	info_list;
	CARDCATS	&cardcats = abgroup.GetCardCatalogs();
	STRING		title;
	STRING		version;
	int		i, j;
	ERRSTK		err;


	assert(objstate.IsReady());
	assert(winlist != NULL);
	assert(blank_glyph != NULL  &&  checkmark_glyph != NULL);
	DbgFunc("ABBROWSER::Update" << endl);


	notify->Busy(gettext("loading Bookshelf list..."));


	// Get list of available AnswerBooks from card catalogs.
	//
	if (cardcats.GetAll(info_list, err)  !=  STATUS_OK) {
		err.Push(gettext("Can't get list of available Bookshelves"));
		notify->ShowErrs(err);
		notify->Done();
		return;
	}

	SortList(info_list, SortEntriesByTitle);


	// Put list in batch update mode to avoid ugly flashing effects.
	//
	winlist->BeginBatch();


	// Clear current list contents, if any.
	//
	winlist->Clear();
	ablist.Clear();


	// Display AnswerBook titles & version numbers in list.
	// Store ABINFO object pointers with corresponding list entries
	// for later use.
	//
	for (i = 0; i < info_list.Count(); i++) {

		title   = info_list[i]->Title();
		version = info_list[i]->Name().ABVersion();
		if (title == NULL_STRING)
			title = gettext("(untitled Bookshelf)");
		if (version != NULL_STRING) {
			title += "   (";
			title += gettext("version");
			title += " " + version + ")";
		}

		winlist->AppendEntry(title, blank_glyph);
		ablist.Add(info_list[i]->Name());
	}


	// Indicate AnswerBooks in list that are currently loaded.
	//
	for (i = 0; i < abgroup.answerbooks.Count(); i++) {
		for (j = 0; j < ablist.Count(); j++) {
			if (abgroup.answerbooks[i]->Name() == ablist[j]) {
				winlist->SetSelection(j);
				winlist->UpdateEntry(j, checkmark_glyph);
				break;
			}
		}
	}


	winlist->EndBatch();
	notify->Done();
}

// Display Browser window.
//
void
ABBROWSER::Show()
{
	assert(objstate.IsReady());
	assert(winlist != NULL);
	DbgFunc("ABBROWSER::Show" << endl);

	xv_set(frame, XV_SHOW, TRUE, NULL);
	xv_set(winlist->XvHandle(), XV_SHOW, TRUE, NULL);
}

// Dismiss Browser window.
//
void
ABBROWSER::Dismiss()
{
	assert(objstate.IsReady());
	assert(frame != XV_NULL);
	DbgFunc("ABBROWSER::Dismiss" << endl);

	xv_set(frame, XV_SHOW, FALSE, XV_NULL);
}

// Dispatch events to the event handler.
//
void
ABBROWSER::DispatchEvent(int event, caddr_t event_obj, caddr_t client_data)
{
	ABBROWSER	*abbrowser = (ABBROWSER *)client_data;

	assert(abbrowser != NULL);
	abbrowser->EventHandler(event, event_obj);
}

// Handle Browser UI events.
//
void
ABBROWSER::EventHandler(int event, caddr_t /*event_obj*/)
{
	int	entry;
	BOOL	busy = BOOL_FALSE;


	assert(objstate.IsReady());
	DbgFunc("ABBROWSER::EventHandler: " << event << endl);


	switch (event) {

	case BROWSER_APPLY_EVENT:
		notify->Busy(gettext("Updating Library . . ."));
		busy = BOOL_TRUE;
		// fall thru to BROWSER_RESET_EVENT...

	case BROWSER_RESET_EVENT:
		// User just pressed the "Apply" or "Reset" button.
		// Pass this event on to interested parties.
		break;

	case BROWSER_RESIZE_EVENT:
		EVENTLOG("resize browser window");
		Resize();
		return;	// don't pass on this event

	case WINLIST_SELECT_EVENT:
	case WINLIST_EXECUTE_EVENT:
		// Whenever user selects an entry, mark it with
		// a checkmark.
		entry = winlist->LastEntrySelected();
		assert(entry >= 0  &&  entry < winlist->NumEntries());
		assert(winlist->IsSelected(entry));
		winlist->UpdateEntry(entry, checkmark_glyph);
		return;	// don't pass on this event

	case WINLIST_DESELECT_EVENT:
		// Whenever user deselects an entry, remove the checkmark.
		entry = winlist->LastEntrySelected();
		assert(entry >= 0  &&  entry < winlist->NumEntries());
		assert( ! winlist->IsSelected(entry));
		winlist->UpdateEntry(entry, blank_glyph);
		return;	// don't pass on this event

	default:
		assert(0);
		break;
	}


	// Pass on event to registered event handler.
	//
	if (event_handler != NULL)
		(*event_handler)(event, (caddr_t)this, event_client_data);
	if (busy)
		notify->Done();
}


// Get list of currently selected AnswerBooks.
//
void
ABBROWSER::GetSelection(LIST<ABNAME> &answerbooks)
{
	int	i;

	assert(objstate.IsReady());
	DbgFunc("ABBROWSER::GetSelection" << endl);


	for (i = 0; i < ablist.Count(); i++) {
		if (winlist->IsSelected(i))
			answerbooks.Add(ablist[i]);
	}
}

// User just hit either the "Apply" or "Reset" button...
//
void
ABBROWSER::ButtonEvent(Panel_item button, Event *)
{
	caddr_t		browser;
	Xv_opaque	panel;
	BROWSER_EVENT	event;

	assert(button != NULL);
	panel = (Panel) xv_get(button, XV_OWNER);
	assert(panel != NULL);
	browser = (caddr_t) xv_get(panel, WIN_CLIENT_DATA);
	event = (BROWSER_EVENT) xv_get(button, PANEL_CLIENT_DATA);

	// By setting button's PANEL_NOTIFY_STATUS to XV_ERROR,
	// we prevent XView from taking down the Browser popup.
	// We'll do this ourselves manually (later).
	//
	(void) xv_set(button, PANEL_NOTIFY_STATUS, XV_ERROR, NULL);

	DispatchEvent(event, NULL, browser);
}

// Adjust the widget layout because user changed size of Browser window.
//
void
ABBROWSER::Resize()
{
	int	button_x, button_y;
	int	button_width;
	int	button_height;
	int	panel_width;
	int	panel_height;
	int	list_width;
	int	list_height;


	assert(objstate.IsReady());
	DbgFunc("ABBROWSER::Resize" << endl);


	panel_width   = (int) xv_get(panel, XV_WIDTH);
	panel_height  = (int) xv_get(panel, XV_HEIGHT);
	button_width  = (int) xv_get(apply_button, XV_WIDTH);
	button_height = (int) xv_get(apply_button, XV_HEIGHT);


	// Center Apply and Reset buttons at bottom of panel.
	//
	button_y = panel_height  - button_height - 10;
	button_x = panel_width/2 - button_width  - 5;
	xv_set(apply_button, XV_X, button_x, XV_Y, button_y, NULL);

	button_x = panel_width/2 + 5;
	xv_set(reset_button, XV_X, button_x, XV_Y, button_y, NULL);


	// Fit AnswerBook list into the remaining space.
	//
	list_height  = button_y;
	list_height -= 10;
	list_height -= (int) xv_get(winlist->XvHandle(), XV_Y);
	list_width   = panel_width - 20;
	winlist->Fit(list_width, list_height);


	// We must redraw widgets here or else the panel will be a mess...
	//
	panel_paint(panel, PANEL_CLEAR);
}

Notify_value
ABBROWSER::PanelEvent(Panel panel, Event *event, Notify_arg arg, 
		   Notify_event_type type)
{
	caddr_t	browser;

	if (event_action(event) != WIN_RESIZE) {

		return(notify_next_event_func(	panel,
						(Notify_event) event, 
						arg,
						type));
	}

	DbgFunc("ABBROWSER::PanelEvent: resize" << endl);

	browser = (caddr_t) xv_get(panel, WIN_CLIENT_DATA);
	DispatchEvent(BROWSER_RESIZE_EVENT, NULL, browser);

	return(NOTIFY_DONE);
}
