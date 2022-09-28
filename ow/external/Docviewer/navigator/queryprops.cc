#ident "@(#)queryprops.cc	1.72 96/11/15 Copyright 1990-1992 Sun Microsystems, Inc."

#include "queryprops.h"
#include "winlist.h"
#include <doc/abclient.h>
#include <doc/abgroup.h>
#include <doc/notify.h>
#include <doc/query.h>
#include <doc/searcher.h>
#include <locale.h>


// XView widget event handler.
//
static void		QueryPropsWidgetEvent(Panel_item, Event *);
static Notify_value	PanelEvent(Panel, Event *, Notify_arg, Notify_event_type);


static int
Max(int a, int b)
{
	return(a > b ? a : b);
}


QUERYPROPS::QUERYPROPS(	Xv_opaque	parent_frame,
			QUERY		*query_arg,
			ABGROUP		*abgroup_arg) :
	query			(query_arg),
	abgroup			(abgroup_arg),
	//	query			(NULL),
	event_handler		(NULL),
	event_client_data	(NULL)
{
	STRING	text_and_titles_lbl,
		titles_only_lbl,
		search_type_lbl,
		sort_by_lbl,
		rank_only_lbl,
		rank_and_book_lbl,
		max_hits_lbl,
		per_answerbook;


	DbgFunc("QUERYPROPS::QUERYPROPS" << endl);
	objstate.MarkGettingReady();


	// Create popup properties frame.
	//
	frame = xv_create(parent_frame, FRAME_PROPS,
			FRAME_SHOW_FOOTER,		TRUE,
			FRAME_SHOW_RESIZE_CORNER,	FALSE,
			WIN_USE_IM,			FALSE,
			NULL);

	if (frame == NULL)
		OutOfMemory();


	// Create status & error message handler.
	//
	notify = new NOTIFY(frame);
	notify->Title(gettext("AnswerBook Navigator: Search Settings"));


	if ((panel = xv_get(frame, FRAME_CMD_PANEL))  ==  NULL)
		OutOfMemory();


	// Initialize panel.
	//
	xv_set(panel,	XV_HELP_DATA,		QUERYPROPS_HELP,
			PANEL_LAYOUT,		PANEL_HORIZONTAL,
			WIN_CLIENT_DATA,	(caddr_t) this,
			NULL);


	// Create Search Type toggle.
	//
	text_and_titles_lbl = gettext("Text and Titles");
	titles_only_lbl     = gettext("Titles Only");
	search_type_lbl     = gettext("Search Library by:");

	titles_toggle = xv_create(panel, PANEL_CHOICE,
			PANEL_CHOICE_NROWS,	1,
			PANEL_CHOICE_STRINGS,
				~text_and_titles_lbl,
				~titles_only_lbl,
				NULL,
			PANEL_LABEL_STRING,	~search_type_lbl,
			PANEL_NOTIFY_PROC,	QueryPropsWidgetEvent,
			PANEL_CLIENT_DATA,	QUERYPROPS_TITLES_TOGGLE_EVENT,
			XV_HELP_DATA,		QUERYPROPS_SCOPE_TOGGLE_HELP,
			NULL);


	// Create "Sort By" toggle.
	//
	sort_by_lbl       = gettext("Sort findings by:");
	rank_only_lbl     = gettext("Relevance Only");
	rank_and_book_lbl = gettext("Relevance and Book");

	sort_toggle = xv_create(panel, PANEL_CHOICE,
			PANEL_CHOICE_NROWS,	1,
			PANEL_CHOICE_STRINGS,
				~rank_only_lbl,
				~rank_and_book_lbl,
				NULL,
			PANEL_LABEL_STRING,	~sort_by_lbl,
			PANEL_NOTIFY_PROC,	QueryPropsWidgetEvent,
			PANEL_CLIENT_DATA,	QUERYPROPS_SORT_TOGGLE_EVENT,
			XV_HELP_DATA,		QUERYPROPS_SCOPE_LIST_HELP,
			NULL);


	// Create "Max Documents" widget.
	//
	max_hits_lbl = gettext("Limit findings to:");

	max_hits_widget = xv_create(panel, PANEL_NUMERIC_TEXT,
		        PANEL_VALUE_DISPLAY_LENGTH, 4,
		        PANEL_VALUE_STORED_LENGTH,  4,
		        PANEL_MIN_VALUE,	10,
		        PANEL_MAX_VALUE,	100,
		        PANEL_LABEL_STRING,	~max_hits_lbl,
			PANEL_NOTIFY_PROC,	QueryPropsWidgetEvent,
			PANEL_NOTIFY_LEVEL,	PANEL_ALL,
			PANEL_CLIENT_DATA,	QUERYPROPS_MAX_HITS_EVENT,
			XV_HELP_DATA,		QUERYPROPS_TITLES_TOGGLE_HELP,
			NULL);

	per_answerbook = gettext("(per AnswerBook)");

	per_ab_widget = xv_create(panel, PANEL_MESSAGE,
		        PANEL_LABEL_STRING,	~per_answerbook,
			NULL);


	// Create Reset and Apply buttons.
	// The X positions are set to 0 as a temporary measure to make sure
	// that when we window_fit_width the buttons do not stretch the window.
	// The final positions will be set after the window_fit_width.
	//
	apply_button = xv_create(panel, PANEL_BUTTON,
			PANEL_LABEL_STRING,	gettext("Apply"),
			PANEL_CLIENT_DATA,	QUERYPROPS_APPLY_EVENT,
			PANEL_NOTIFY_PROC,	QueryPropsWidgetEvent,
			XV_HELP_DATA,		QUERYPROPS_APPLY_BUTTON_HELP,
			XV_X, 0,
			NULL);

	reset_button = xv_create(panel, PANEL_BUTTON,
			PANEL_LABEL_STRING,	gettext("Reset"),
			PANEL_CLIENT_DATA,	QUERYPROPS_RESET_EVENT,
			PANEL_NOTIFY_PROC,	QueryPropsWidgetEvent,
			XV_HELP_DATA,		QUERYPROPS_RESET_BUTTON_HELP,
			XV_X, 0,
			NULL);


	// Make sure it's all there.
	//
	if (titles_toggle     == NULL  ||
	    apply_button      == NULL  ||
	    reset_button      == NULL) {
		OutOfMemory();
	}


	// Lay out widgets on panel.
	//
	PanelLayout();


	// We're ready to roll...
	//
	objstate.MarkReady();
}

QUERYPROPS::~QUERYPROPS()
{
	DbgFunc("QUERYPROPS::QUERYPROPS" << endl);
	if (notify)
	   	delete notify;
}

void
QUERYPROPS::Show()
{
	assert(objstate.IsReady());
	DbgFunc("QUERYPROPS::Show" << endl);
	EVENTLOG("search props");

	Update();
	xv_set(frame, XV_SHOW, TRUE, NULL);
}

void
QUERYPROPS::Update()
{
	assert(objstate.IsReady());
	DbgFunc("QUERYPROPS::Update" << endl);

	ResetProps();
}

// Dispatch events to the event handler.
//
void
QUERYPROPS::DispatchEvent(int event, caddr_t event_obj, caddr_t client_data)
{
	QUERYPROPS	*qp = (QUERYPROPS *)client_data;

	assert(qp != NULL);
	qp->EventHandler(event, event_obj);
}

// Handle Query Props events.
//
void
QUERYPROPS::EventHandler(int event, caddr_t /*event_obj*/)
{
	assert(objstate.IsReady());
	DbgFunc("QUERYPROPS::EventHandler: " << event << endl);


	switch (event) {

	case QUERYPROPS_TITLES_TOGGLE_EVENT:
		// nothing to do here...
		break;

	case QUERYPROPS_APPLY_EVENT:
		notify->Busy("");
		if (ApplyProps() != STATUS_OK) {

			// There was a problem applying the properties.
			// By setting button's PANEL_NOTIFY_STATUS to XV_ERROR,
			// we prevent XView from taking down the props sheet
			// so that the user can resolve the problem.
			//
			xv_set(apply_button,
				PANEL_NOTIFY_STATUS, XV_ERROR,
				NULL);
		}
		notify->Done();
		break;

	case QUERYPROPS_RESET_EVENT:
		notify->Busy("");
		ResetProps();

		// By setting button's PANEL_NOTIFY_STATUS to XV_ERROR,
		// we prevent XView from taking down the props sheet.
		//
		xv_set(reset_button, PANEL_NOTIFY_STATUS, XV_ERROR, NULL);
		notify->Done();
		break;


	default:
		//XXX ???
		break;
	}
}

// Apply property settings to the query object.
//
STATUS
QUERYPROPS::ApplyProps()
{
	STRING	alert_msg;
	int	max_hits;


	assert(objstate.IsReady());
	DbgFunc("QUERYPROPS::ApplyProps" << endl);


	// Set Text'n'Titles/Titles only state.
	//
	if ((int) xv_get(titles_toggle, PANEL_VALUE)  ==  0) {
		query->TitlesOnly(BOOL_FALSE);
	} else {
		query->TitlesOnly(BOOL_TRUE);
	}


	//XXX Set hitlist sort order.
	//
	switch ((int) xv_get(sort_toggle, PANEL_VALUE)) {
	case 1:
		query->SortOrder(SORT_BY_BOOK);
		break;
	case 0:
	default:
		query->SortOrder(SORT_BY_RANK);
		break;
	}


	// Set "Max Hits".
	//
	max_hits = (int) xv_get(max_hits_widget, PANEL_VALUE);
	query->MaxDocuments(max_hits);


	return(STATUS_OK);
}

// Set all of the panel items to match the settings in the query object.
//
void
QUERYPROPS::ResetProps()
{
	BOOL	titles_only = query->TitlesOnly();
//XXX	BOOL	sort_order  = query->TitlesOnly();
	int	max_hits    = query->MaxDocuments();


	assert(objstate.IsReady());
	DbgFunc("QUERYPROPS::ResetProps" << endl);


	// Reset "Titles Only" toggle, "Sort By" toggle,
	// and "Max Documents" selector.
	//
	xv_set(titles_toggle, PANEL_VALUE, titles_only, NULL);
	xv_set(max_hits_widget, PANEL_VALUE, max_hits, NULL);

	switch (query->SortOrder()) {
	case SORT_BY_RANK:
		xv_set(sort_toggle, PANEL_VALUE, 0, NULL);
		break;
	case SORT_BY_BOOK:
		xv_set(sort_toggle, PANEL_VALUE, 1, NULL);
		break;
	}
}

// Notify callback function for Query Props widgets.
//
void
QueryPropsWidgetEvent(Panel_item widget, Event *)
{
	Xv_opaque	panel;
	caddr_t		queryprops;
	int		event;

	DbgFunc("QueryPropsWidgetEvent" << endl);

	event      = (int)       xv_get(widget, PANEL_CLIENT_DATA);
	panel      = (Xv_opaque) xv_get(widget, XV_OWNER);
	queryprops = (caddr_t)   xv_get(panel,  WIN_CLIENT_DATA);

	QUERYPROPS::DispatchEvent(event, NULL, queryprops);
}

void
QUERYPROPS::PanelLayout()
{
	int	panel_width;
	int	button_width;
	int	widget_x, widget_y;
	int	label_width, max_label_width;


	assert(objstate.IsReady());
	assert(panel != NULL);
	DbgFunc("QUERYPROPS::Resize" << endl);
	EVENTLOG("search props resize");


	// Arrange all widgets except for the Apply and Reset buttons
	// XXX...
	max_label_width = (int) xv_get(titles_toggle, PANEL_LABEL_WIDTH);
	label_width     = (int) xv_get(sort_toggle, PANEL_LABEL_WIDTH);
	max_label_width = Max(label_width, max_label_width);
	label_width     = (int) xv_get(max_hits_widget, PANEL_LABEL_WIDTH);
	max_label_width = Max(label_width, max_label_width);

	widget_x = max_label_width + 10;
	widget_y = 10;
	xv_set(titles_toggle, PANEL_VALUE_X, widget_x, XV_Y, widget_y, NULL);

	widget_y += (int) xv_get(titles_toggle, XV_HEIGHT) + 10;
	xv_set(sort_toggle, PANEL_VALUE_X, widget_x, XV_Y, widget_y, XV_NULL);

	widget_y += (int) xv_get(sort_toggle, XV_HEIGHT) + 15;
	xv_set(max_hits_widget, PANEL_VALUE_X, widget_x, XV_Y, widget_y, 0);

	widget_x = (int) xv_get(max_hits_widget, XV_X) +
				(int) xv_get(max_hits_widget, XV_WIDTH) + 15;
	xv_set(per_ab_widget, XV_X, widget_x, XV_Y, widget_y, 0);


	// Now fit the panel width-wise around these widgets.
	//
	window_fit_width(panel);


	// Center Apply and Reset buttons 20 pixels below the
	// Max Hits selector with a 10 pixel gap between them.
	//
	panel_width   = (int) xv_get(panel, XV_WIDTH);
	widget_y     += (int) xv_get(max_hits_widget, XV_HEIGHT) + 20;
	button_width  = (int) xv_get(apply_button, XV_WIDTH);
	widget_x      = panel_width/2 - button_width - 5;
	xv_set(apply_button, XV_X, widget_x, XV_Y, widget_y, NULL);
	widget_x = panel_width/2 + 5;
	xv_set(reset_button, XV_X, widget_x, XV_Y, widget_y, NULL);


	// Fit panel and frame around widgets.
	//
	window_fit(panel);
	window_fit(frame);


	// We must redraw widgets here or else the panel will be a mess...
	//
	panel_paint(panel, PANEL_NO_CLEAR);
}
