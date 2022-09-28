#ident "@(#)winlist.cc	1.3 03/09/93 Copyright 1990-1992 Sun Microsystems, Inc."

#include "winlist.h"
#include <doc/itimer.h>
#include <doc/console.h>
#include <locale.h>

extern "C" {
	int ds_is_double_click(Event *previousEvent, Event *event);
};

extern void	NewHandler();

// WINLIST constructor.
//
WINLIST::WINLIST(Xv_opaque panel, int x, int y) :
	num_entries		(0),
	last_entry_selected	(-1),
	event_handler		(NULL),
	event_arg		(NULL),
	batch_mode		(BOOL_FALSE),
	batch_index		(0)
{
	DbgFunc("WINLIST::WINLIST" << endl);


	// Create XView scrolling list object.
	// Note that initially XV_SHOW is false.
	//
	win_list = xv_create(panel, PANEL_LIST,
			XV_X,			x,
			XV_Y,			y,
			XV_SHOW,		FALSE,
			PANEL_CHOOSE_NONE,	FALSE,
			PANEL_CHOOSE_ONE,	TRUE,
			PANEL_READ_ONLY,	TRUE,
			PANEL_NOTIFY_PROC,	WINLIST::NotifyProc,
			PANEL_CLIENT_DATA,	(caddr_t) this,
			NULL);


	// Find the bold and regular fonts for this panel.
	// We use these to create bold entry titles.
	//
	if ((regular_font = (Xv_Font) xv_get(panel, XV_FONT))  ==  XV_NULL)
		NewHandler();

	bold_font = (Xv_Font) xv_find(panel, FONT,
			FONT_FAMILY,	xv_get(regular_font, FONT_FAMILY),
			FONT_SIZE,	xv_get(regular_font, FONT_SIZE),
			FONT_STYLE,	FONT_STYLE_BOLD,
			NULL);

	if (bold_font == XV_NULL)
		bold_font = regular_font;


	// Get list's scrollbar.
	//
	scrollbar = (Scrollbar) xv_get(win_list, PANEL_LIST_SCROLLBAR);


	// Create timer object.
	//
	timer = new ITIMER();


	// Make sure we've got everything we need.
	//
	if (win_list     == NULL  ||
	    scrollbar    == NULL  ||
	    bold_font    == NULL  ||
	    regular_font == NULL)
		NewHandler();


	(void) memset((void *) &prevEvent, NULL, sizeof(Event));


	// We're ready to roll...
	//
	objstate.MarkReady();
}

WINLIST::~WINLIST()
{
	DbgFunc("WINLIST::~WINLIST" << endl);

	if (timer)
		delete timer;
}

// Add entry to list.
//
void
WINLIST::InsertEntry(int entry, const STRING &label, BOOL bold, GLYPH glyph)
{
	Xv_font		font = (bold ? bold_font : regular_font);
	int		n;

	assert(objstate.IsReady());
	assert(font != NULL);
	DbgFunc("WINLIST::InsertEntry " << label << endl);


	// Make a copy of the label string.
	//
	n = batch_strings.Count();
	batch_strings.Add(label);

	batch_attrs[batch_index++] = (void *) PANEL_LIST_STRING;
	batch_attrs[batch_index++] = (void *) entry;
	batch_attrs[batch_index++] = (void *) ~batch_strings[n];
	batch_attrs[batch_index++] = (void *) PANEL_LIST_FONT;
	batch_attrs[batch_index++] = (void *) entry;
	batch_attrs[batch_index++] = (void *) font;

	if (glyph != NULL) {
		batch_attrs[batch_index++] = (void *) PANEL_LIST_GLYPH;
		batch_attrs[batch_index++] = (void *) entry;
		batch_attrs[batch_index++] = (void *) glyph;
	}

	++num_entries;
	BatchUpdate();
}

// Update specified list entry (version 1 - update string only)
//
void
WINLIST::UpdateEntry(int entry, const STRING &label, BOOL bold)
{
	Xv_font	font = (bold ? bold_font : regular_font);
	int	n;

	assert(objstate.IsReady());
	assert(entry >= 0  &&  entry < NumEntries());
	DbgFunc("WINLIST::UpdateEntry " << label << endl);


	// Make a copy of the label string.
	//
	n = batch_strings.Count();
	batch_strings.Add(label);

	batch_attrs[batch_index++] = (void *) PANEL_LIST_STRING;
	batch_attrs[batch_index++] = (void *) entry;
	batch_attrs[batch_index++] = (void *) ~batch_strings[n];
	batch_attrs[batch_index++] = (void *) PANEL_LIST_FONT;
	batch_attrs[batch_index++] = (void *) entry;
	batch_attrs[batch_index++] = (void *) font;

	BatchUpdate();
}

// Update specified list entry (version 2 - update glyph only)
//
void
WINLIST::UpdateEntry(int entry, GLYPH glyph)
{
	assert(objstate.IsReady());
	assert(entry >= 0  &&  entry < NumEntries());
	DbgFunc("WINLIST::UpdateEntry" << endl);

	batch_attrs[batch_index++] = (void *) PANEL_LIST_GLYPH;
	batch_attrs[batch_index++] = (void *) entry;
	batch_attrs[batch_index++] = (void *) glyph;

	BatchUpdate();
}

// Delete entry from list.
//
void
WINLIST::DeleteEntry(int entry)
{
	assert(objstate.IsReady());
	assert(entry >= 0  &&  entry < NumEntries());
	DbgFunc("WINLIST::DeleteEntry\n");

	batch_attrs[batch_index++] = (void *) PANEL_LIST_DELETE;
	batch_attrs[batch_index++] = (void *) entry;

	--num_entries;
	BatchUpdate();
}

// Delete multiple entries from list.
//
void
WINLIST::DeleteEntries(int start, int count)
{
	assert(objstate.IsReady());
	assert(start >= 0  &&  count >= 0);
	assert(start       <  NumEntries());
	assert(start+count <= NumEntries());
	DbgFunc("WINLIST::DeleteEntry\n");
	
	batch_attrs[batch_index++] = (void *) PANEL_LIST_DELETE_ROWS;
	batch_attrs[batch_index++] = (void *) start;
	batch_attrs[batch_index++] = (void *) count;

	num_entries -= count;
	BatchUpdate();
}

// Associate an arbitrary cookie with the specified entry.
//
void
WINLIST::SetClientData(int entry, caddr_t cookie)
{
	assert(entry >= 0  &&  entry < NumEntries());
	DbgFunc("WINLIST::SetClientData: " << entry << endl);

	batch_attrs[batch_index++] = (void *) PANEL_LIST_CLIENT_DATA;
	batch_attrs[batch_index++] = (void *) entry;
	batch_attrs[batch_index++] = (void *) cookie;

	BatchUpdate();
}

// Retrieve the cookie associated with the specified entry.
//
caddr_t
WINLIST::GetClientData(int entry) const
{
	assert(objstate.IsReady());
	assert(entry >= 0  &&  entry < NumEntries());
	assert( ! batch_mode);
	DbgFunc("WINLIST::GetClientData" << endl);

	return((caddr_t) xv_get(win_list, PANEL_LIST_CLIENT_DATA, entry));
}

// Clear the list (delete all entries).
//
void
WINLIST::Clear()
{
	assert(objstate.IsReady());
	DbgFunc("WINLIST::Clear" << endl);


	batch_attrs[batch_index++] = (void *) PANEL_LIST_DELETE_ROWS;
	batch_attrs[batch_index++] = (void *) 0;
	batch_attrs[batch_index++] = (void *) NumEntries();

	num_entries = 0;
	BatchUpdate();
}

// Get the entry number of the currently selected entry.
// Returns -1 if there are no entries.
//
int
WINLIST::GetSelection() const
{
	assert(objstate.IsReady());
	assert( ! batch_mode);

	return((int)xv_get(win_list, PANEL_LIST_FIRST_SELECTED));
}

// Set the current selection to 'entry'.
//
void
WINLIST::SetSelection(int entry)
{
	assert(objstate.IsReady());
	assert(entry >= 0  &&  entry < NumEntries());

	batch_attrs[batch_index++] = (void *) PANEL_LIST_SELECT;
	batch_attrs[batch_index++] = (void *) entry;
	batch_attrs[batch_index++] = (void *) TRUE;

	BatchUpdate();
}

// Select the specified entry.
//
void
WINLIST::Select(int entry)
{
	assert(objstate.IsReady());
	assert(entry >= 0  &&  entry < NumEntries());

	batch_attrs[batch_index++] = (void *) PANEL_LIST_SELECT;
	batch_attrs[batch_index++] = (void *) entry;
	batch_attrs[batch_index++] = (void *) TRUE;

	BatchUpdate();
}

// Deselect the specified entry.
//
void
WINLIST::Deselect(int entry)
{
	assert(objstate.IsReady());
	assert(entry >= 0  &&  entry < NumEntries());

	batch_attrs[batch_index++] = (void *) PANEL_LIST_SELECT;
	batch_attrs[batch_index++] = (void *) entry;
	batch_attrs[batch_index++] = (void *) FALSE;

	BatchUpdate();
}

// Is the specified entry selected?
//
BOOL
WINLIST::IsSelected(int entry) const
{
	assert(objstate.IsReady());
	assert(entry >= 0  &&  entry < NumEntries());
	assert( ! batch_mode);
	DbgFunc("WINLIST::IsSelected: " << entry << endl);

	if (xv_get(win_list, PANEL_LIST_SELECTED, entry))
		return(BOOL_TRUE);
	else
		return(BOOL_FALSE);
}

// Fit list into specified width.
//
void
WINLIST::FitWidth(int width)
{
	DbgFunc("WINLIST::FitWidth: " << width << endl);

	width -= (int) xv_get(scrollbar, XV_WIDTH);
	if (width < 100)
		width = 100;

	batch_attrs[batch_index++] = (void *) PANEL_LIST_WIDTH;
	batch_attrs[batch_index++] = (void *) width;

	BatchUpdate();
}

// Fit list into specified height.
//
void
WINLIST::FitHeight(int height)
{
	int	nrows;

	DbgFunc("WINLIST::FitHeight: " << height << endl);

	// If list has a label, subtract the label height before
	// calculating how many rows would fit into the specified height.
	//
	if ((char *) xv_get(win_list, PANEL_LABEL_STRING)  !=  NULL) {
		height -= (int) xv_get(bold_font, FONT_SIZE);
		height -= 6;	//XXX fudge
	}

	nrows   = height / (int)xv_get(win_list, PANEL_LIST_ROW_HEIGHT);
	if (nrows < 4)
		nrows = 4;
	SetViewNumRows(nrows - 1);
}

// Set the entry currently at the top of the display window.
//
void
WINLIST::SetViewTop(int entry)
{
	assert(objstate.IsReady());
	assert( ! batch_mode);
	DbgFunc("WINLIST::SetViewTop: " << entry << endl);

	if (entry >= NumEntries())
		entry = NumEntries() - 1;
	else if (entry < 0);
		entry = 0;

	xv_set(scrollbar, SCROLLBAR_VIEW_START, entry, NULL);
}

// Set the entry currently at the bottom of the display window.
//
void
WINLIST::SetViewBottom(int entry)
{
	assert(objstate.IsReady());
	assert( ! batch_mode);
	DbgFunc("WINLIST::SetViewBottom: " << entry << endl);

	SetViewTop(entry + GetViewNumRows());
}

// Get the entry currently at the top of the display window.
//
int
WINLIST::GetViewTop()
{
	assert(objstate.IsReady());
	assert( ! batch_mode);
	DbgFunc("WINLIST::GetViewTop" << endl);

	return( (int) xv_get(scrollbar, SCROLLBAR_VIEW_START));
}

// Get the entry currently at the bottom of the display window.
//
int
WINLIST::GetViewBottom()
{
	assert(objstate.IsReady());
	assert( ! batch_mode);
	DbgFunc("WINLIST::GetViewBottom" << endl);

	return(GetViewTop() + GetViewNumRows());
}

// Is entry currently in display window?
//
BOOL
WINLIST::InView(int n)
{
	assert(objstate.IsReady());
	assert( ! batch_mode);
	DbgFunc("WINLIST::InView: " << n << endl);

	if (n >= GetViewTop()  &&  n <= GetViewBottom())
		return(BOOL_TRUE);
	else
		return(BOOL_FALSE);
}

// Get/set number of rows currently visible in list.
//
int
WINLIST::GetViewNumRows()
{
	assert(objstate.IsReady());
	DbgFunc("WINLIST::GetViewNumRows" << endl);

	return((int) xv_get(win_list, PANEL_LIST_DISPLAY_ROWS));
}

void
WINLIST::SetViewNumRows(int n)
{
	assert(objstate.IsReady());
	DbgFunc("WINLIST::SetViewNumRows: " << n << endl);

	batch_attrs[batch_index++] = (void *) PANEL_LIST_DISPLAY_ROWS;
	batch_attrs[batch_index++] = (void *) n;

	BatchUpdate();
}

// Selection callback routine for single and double click panel list events.
// XXX	The TimeOut is a workaround for an XView panel list selection bug.
//	Otherwise, we would simply invoke the callbacks directly from here.
//
void
WINLIST::NotifyProc(	Panel_list_item	item,
			char		*,
			Xv_opaque	/*client_data*/,
			Panel_list_op	op,
			Event		*event,
			int		entry)
{
	WINLIST	*winlist = (WINLIST *) xv_get(item, PANEL_CLIENT_DATA);


	assert(winlist != NULL);
	assert(winlist->objstate.IsReady());


	switch (op) {

	case PANEL_LIST_OP_SELECT:
	        if (ds_is_double_click(&winlist->prevEvent, event)) {
			// This is a double click ("execute") event.
			// Clear out the event structure so the next event
			// won't also be considered a double click.
			//
        	        winlist->XXXevent = WINLIST_EXECUTE_EVENT;
			memset((void *) &winlist->prevEvent, 0, sizeof(Event));
	        } else {
			// This is a "select" event.
			// Save the event structure, and check for a
			// double click when the next event arrives.
			//
			winlist->prevEvent = *event;
	                winlist->XXXevent = WINLIST_SELECT_EVENT;
	        }

		// Remember which entry was selected for future reference.
		//
		winlist->last_entry_selected = entry;
		break;

	case PANEL_LIST_OP_DESELECT:
                winlist->XXXevent = WINLIST_DESELECT_EVENT;

		// Forget about this event - we dont' want the next event
		// interpreted as a double click...
		//
		memset((void *) &winlist->prevEvent, 0, sizeof(Event));

		// Remember which entry was deselected for future reference.
		//
		winlist->last_entry_selected = entry;
		break;

	case PANEL_LIST_OP_DELETE:
	case PANEL_LIST_OP_VALIDATE:
		// This better not happen - we're not prepared for it.
		//
		assert(0);
		return;

	default:
		return;
	}


	if (winlist->event_handler != NULL) {
		//XXX workaround for XView panel list bug
		winlist->timer->TimeOut(0, 1000,
				(TimerCallBack) WINLIST::XXXTimerEvent,
				(caddr_t) winlist);
//XXX		(*winlist->event_handler)(event, winlist, winlist->event_arg);
	}
}

void
WINLIST::XXXTimerEvent(caddr_t client_data)
{
	WINLIST	*winlist = (WINLIST *) client_data;

	DbgFunc("WINLIST::XXXTimerEvent" << endl);
	assert(winlist != NULL  &&  winlist->objstate.IsReady());

	if (winlist->event_handler != NULL)
		(*winlist->event_handler)(winlist->XXXevent,
					  (caddr_t)winlist,
					  winlist->event_arg);
}

void
WINLIST::BeginBatch()
{
	assert(objstate.IsReady());
	assert( ! batch_mode);
	DbgFunc("WINLIST::BeginBatch" << endl);

	batch_mode  = BOOL_TRUE;
	batch_index = 0;

	xv_set(win_list, XV_SHOW, BOOL_FALSE, NULL);
}

void
WINLIST::EndBatch()
{
	assert(objstate.IsReady());
	assert(batch_mode);
	DbgFunc("WINLIST::EndBatch" << endl);

	batch_mode = BOOL_FALSE;
	BatchUpdate();

	xv_set(win_list, XV_SHOW, BOOL_TRUE, NULL);
}

void
WINLIST::BatchUpdate()
{
	assert(objstate.IsReady());
	DbgFunc("WINLIST::BatchUpdate" << endl);


	// Update the list now if we're not actually running in batch mode,
	// or if we're about to overflow the batch attribute list.
	//
	if ( ! batch_mode  ||  batch_index > (ATTR_STANDARD_SIZE - 10)) {

		batch_attrs[batch_index] = NULL;
		xv_set(win_list, ATTR_LIST, batch_attrs, NULL);
		batch_index = 0;
		batch_strings.Clear();

		// Compare how many list entries we think we have to
		// how many the list thinks it has.
		// They should always be the same at this point.
		//
		int	n = (int) xv_get(win_list, PANEL_LIST_NROWS);
		assert(n == num_entries);
	}
}
