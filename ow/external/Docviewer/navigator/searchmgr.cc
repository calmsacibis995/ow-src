#ident "@(#)searchmgr.cc	1.62 95/02/13 Copyright 1990-1992 Sun Microsystems, Inc."

#define	_OTHER_TEXTSW_FUNCTIONS	//XXX see <xview/textsw.h> for clues

#include "searchmgr.h"
#include "hitlist.h"
#include "queryhist.h"
#include "queryprops.h"
#include "inputwin.h"
#include "xview.h"
#include <doc/abclient.h>
#include <doc/abgroup.h>
#include <doc/notify.h>
#include <doc/query.h>
#include <doc/searcher.h>
#include <doc/searchdoc.h>
#include <locale.h>


static const int	MAX_QUERY_SIZE(1024);

static const int WINLIST_TOP = 0;

// Unique key for associating SEARCHMGR object with XView object.
// This allows XView event handlers to retrieve the SEARCHMGR object
// from the XView object.
//
static int	SEARCHMGR_KEY   = (int) xv_unique_key();


// Intercept XView events from buttons and pass them on
// to main  "EventHandler()".
//
static void	ButtonEvent(Panel_item button, Event *event);

// Intercept XView events from Query Window and pass them on
// to main  "EventHandler()".
//
static Notify_value	QueryWinEvent(	Xv_window		win,
					Notify_event		event,
					Notify_arg		arg,
					Notify_event_type	type);


SEARCHMGR::SEARCHMGR(Xv_opaque frame, NOTIFY *notify, ABGROUP *abgroup_arg) :
	MODEMGR		(frame, notify),
	abgroup		(abgroup_arg),
	queryprops	(NULL),
	queryhist	(NULL)
{
	Panel_item	querylabel;	// query window label "Search For:"
	STRING		labelstr;
	int		panel_y;
	int		margin_x, margin_y;
	int		label_x, label_y;
	int		query_x, query_y;
	int		button_x, button_y;
	int		list_x, list_y;
	Xv_window	win;


	assert(objstate.IsReady());	// should be set in MODEMGR::MODEMGR
	assert(mode_panel != NULL);	// ditto
	assert(abgroup != NULL);
	DbgFunc("SEARCHMGR::SEARCHMGR" << endl);


	notify->Busy("");


	xv_set(mode_panel, XV_HELP_DATA, MODE_SEARCH_HELP, NULL);


	margin_x = 10;
	margin_y = 10;


	// Create and label query window.
	// 
	// Create query window label: "Search For:"
	//
	labelstr = gettext("Search Library For:");
	label_x = margin_x;
	label_y = 0;
	querylabel = xv_create(mode_panel, PANEL_MESSAGE,
			XV_X,			label_x,
			XV_Y,			label_y,
			PANEL_LABEL_STRING,	~labelstr,
			PANEL_LABEL_BOLD,	TRUE,
			0);

	query_x = margin_x;
	query_y = (int) xv_get(querylabel, XV_HEIGHT) + 5;
	panel_y = (int) xv_get(mode_panel, XV_Y); // origin relative to frame

	querywin = new INPUTWIN(frame, query_x, query_y+panel_y, 4);
	xv_set(querywin->XvHandle(),
			WIN_DESIRED_WIDTH,	WIN_EXTEND_TO_EDGE,
			XV_NULL);

	querywin->XvHelpData(SEARCH_QUERYWIN_HELP);
	win = querywin->XvViewWindow();


	// Set up to handle carriage returns on the query win (ppk)
	// 
	(void) notify_interpose_event_func((Notify_client) win,
					   (Notify_func) QueryWinEvent,
					   NOTIFY_SAFE);

	(void) xv_set(querywin->XvViewWindow(),
		      WIN_CLIENT_DATA,	(caddr_t) this,
		      XV_NULL);


	// Create a row of three buttons:
	//	Search
	//	Previous Searches...
	//	Search Settings...
	// Position them just below the query window we just created.
	//
	button_x = margin_x;
	button_y = query_y + querywin->XvGet(XV_HEIGHT) + 10;

	xv_set(mode_panel, PANEL_LAYOUT, PANEL_HORIZONTAL, NULL);

	search_button = xv_create(mode_panel, PANEL_BUTTON,
			XV_X,			button_x,
			XV_Y,			button_y,
			XV_HELP_DATA,		SEARCH_FULL_BUTTON_HELP,
			PANEL_LABEL_STRING,	gettext("Start Search"),
			PANEL_NOTIFY_PROC,	ButtonEvent,
			PANEL_CLIENT_DATA,	DO_SEARCH_EVENT,
			NULL);

	button_x += (int) xv_get(search_button, XV_WIDTH) + 20;
	history_button = xv_create(mode_panel, PANEL_BUTTON,
			XV_X,			button_x,
			XV_Y,			button_y,
			XV_HELP_DATA,		SEARCH_HISTORY_BUTTON_HELP,
			PANEL_LABEL_STRING,    gettext("Previous Searches..."),
			PANEL_NOTIFY_PROC,	ButtonEvent,
			PANEL_CLIENT_DATA,	SEARCH_HISTORY_EVENT,
			NULL);

	button_x += (int) xv_get(history_button, XV_WIDTH) + 10;
	props_button = xv_create(mode_panel, PANEL_BUTTON,
			XV_X,			button_x,
			XV_Y,			button_y,
			XV_HELP_DATA,		SEARCH_PROPS_BUTTON_HELP,
			PANEL_LABEL_STRING,	gettext("Search Settings..."),
			PANEL_NOTIFY_PROC,	ButtonEvent,
			PANEL_CLIENT_DATA,	SEARCH_PROPS_EVENT,
			NULL);

	if (search_button  == NULL  ||
	    history_button == NULL  ||
	    props_button   == NULL) {
		OutOfMemory();
	}


	// Associate current SEARCHCTRL object with this panel
	// so we can access it later from event handler.
	//
	xv_set(mode_panel, XV_KEY_DATA, SEARCHMGR_KEY, (caddr_t)this, NULL);


	// Create and label hitlist scrolling list.
	// 
	list_x = margin_x;
	list_y = button_y + (int) xv_get(props_button, XV_HEIGHT) + 20;
	hitlist = new HITLIST(mode_panel, list_x, list_y, abgroup);

	xv_set(hitlist->XvHandle(),
			PANEL_LAYOUT,		PANEL_VERTICAL,
			PANEL_LABEL_STRING,	gettext("Documents Found:"),
			XV_HELP_DATA,		SEARCH_HITLIST_HELP,
			XV_NULL);

	// Register event handler for search hitlist events
	// ("select" and "execute").
	//
	hitlist->SetEventHandler(DispatchEvent, (caddr_t) this);


	// Create new query.
	//
	curr_query = new QUERY();


	// We're ready to roll...
	//
	notify->Done();
}

SEARCHMGR::~SEARCHMGR()
{
	DbgFunc("SEARCHMGR::~SEARCHMGR" << endl);

	if (querywin != NULL)
		delete querywin;
	if (queryprops != NULL)
		delete queryprops;
	if (curr_query != NULL)
		delete curr_query;
	if (queryhist != NULL)
		delete queryhist;
	if (hitlist != NULL)
		delete hitlist;
}

STATUS
SEARCHMGR::Update(ERRSTK &)
{
	assert(objstate.IsReady());
	assert(hitlist != NULL);
	DbgFunc("SEARCHMGR::Update" << endl);

	hitlist->Clear();
	if (queryprops != NULL)
		queryprops->Update();

	return(STATUS_OK);
}

void
SEARCHMGR::Show(BOOL flag)
{
	assert(objstate.IsReady());
	DbgFunc("SEARCHMGR::Show" << endl);

	if (flag) {
		xv_set(hitlist->XvHandle(), XV_SHOW, TRUE, XV_NULL);
		EventHandler(MM_PANEL_RESIZE_EVENT, NULL);
		xv_set(mode_panel, XV_SHOW, TRUE, NULL);

		// Show query window and give it keyboard focus
		// 
		querywin->Show(BOOL_TRUE);
		querywin->XvSetFocus();
	} else {
		xv_set(mode_panel, XV_SHOW, FALSE, NULL);
		querywin->Show(BOOL_FALSE);
		xv_set(hitlist->XvHandle(), XV_SHOW, FALSE, XV_NULL);
	}
}

// Get the currently selected document in the hit list.
// Assumes there is a current selection (see "HasSelection()").
//
const DOCNAME &
SEARCHMGR::GetSelection() const
{
	int	selection;

	assert(objstate.IsReady());
	assert(hitlist != NULL);
	assert(hitlist->NumEntries() > 0);
	DbgFunc("SEARCHMGR::GetSelection" << endl);

	selection = hitlist->GetSelection();
	assert(selection >= 0);

	return(hitlist->GetDocName(selection));
}

// Main event handler.
//
void
SEARCHMGR::EventHandler(int event, caddr_t /*event_obj*/)
{
	int	width, height;	// width, height of hit list.

	assert(objstate.IsReady());
	DbgFunc("SEARCHMGR::EventHandler: " << event << endl);


	switch (event) {

	case WINLIST_SELECT_EVENT:
		// User single-clicked on hitlist entry.
		// We're not interested in this event.
		//
		return;

	case WINLIST_EXECUTE_EVENT:
	case MM_VIEW_EVENT:
		if (hitlist->NumHits() > 0) {
			MODEMGR::DefaultEventHandler(MM_VIEW_EVENT,
						     (caddr_t)this);
		}
		else {
			notify->Warning(gettext("No document to display"));
		}
		break;

	case SEARCH_PROPS_EVENT:
		queryprops = GetQueryProps();
		assert(queryprops != NULL);
		queryprops->Show();
		break;

	case SEARCH_HISTORY_EVENT:
		queryhist = GetQueryHist();
		assert(queryhist != NULL);
		queryhist->Show();
		break;

	case QUERYPROPS_APPLY_EVENT:
		EVENTLOG("apply search props");
		break;

	case QUERYWIN_RETURN_EVENT:
		// Erase the RETURN.
		// 
		textsw_edit(querywin->XvHandle(), TEXTSW_UNIT_IS_CHAR, 1, 1);

		// Select query window text with 'pending delete'
		// so that it will disappear next time the user types
		// in the window.
		// 
//XXX	    textsw_set_selection(querywin->XvHandle(), 0, TEXTSW_INFINITY, 17);
		// Fall through to "DO_SEARCH_EVENT"...

	case DO_SEARCH_EVENT:
		// User initiated search.
		//
		DoSearch();
		break;

	case MM_PANEL_RESIZE_EVENT:
		// Mode panel was resized - adjust layout of panel widgets
		// accordingly.
		// Extend hit list to bottom of mode_panel and make it
		// as wide as the mode_panel.
		// 
		width   = (int) xv_get(mode_panel, XV_WIDTH);
		width  -= (int) xv_get(hitlist->XvHandle(), XV_X);
		height  = (int) xv_get(mode_panel, XV_HEIGHT);
		height -= (int) xv_get(hitlist->XvHandle(), XV_Y);

		hitlist->Fit(width, height);
		break;

	default:
		MODEMGR::DefaultEventHandler(event, (caddr_t)this);
		break;
	}
}

void
SEARCHMGR::DoSearch()
{
	LISTX<SEARCHDOC*>	results;	// results of search
	LISTX<ABCLIENT*>	&answerbooks = abgroup->answerbooks;
	STRING			titlesonly_string;
	ERRSTK			err;
	STATUS			status;
	int			i;
	int			j;
	extern STATUS		XvSleep(int secs, ERRSTK &);


	assert(objstate.IsReady());
	assert(abgroup != NULL);
	DbgFunc("SEARCHMGR::DoSearch" << endl);


	notify->Busy(gettext("Searching AnswerBooks in Library . . ."));
	hitlist->Clear();

	// display list from the top of the scroll bar:

	hitlist->SetViewTop(WINLIST_TOP);

	// Make sure there's something to search.
	//
	if (answerbooks.Count() == 0) {
		notify->Done();
		notify->Warning(gettext(
			"Search unsuccessful: no AnswerBooks in Library"));
		return;
	}

	// Get query text from query window.
	// 
	curr_query->Text(querywin->GetText(MAX_QUERY_SIZE));


	// Workaround for SDR dv237.
	// Query textsw's in-memory edit log was filling up,
	// resulting in impolite alert boxes saying "Insertion failed"
	// whenever user attempted to enter a new query.
	// Calling querywin->SetText() in turn calls textsw_reset(),
	// which clears out textsw's edit log.
	// 
	STRING qtext = querywin->GetText(MAX_QUERY_SIZE);
	querywin->SetText(qtext);

	// Figure out if we can leave collections open or if we
	// need to open/search/close.

	BOOL delete_searcher = BOOL_FALSE;
	// Attempt to open all the answerbook collections
	// If they are already open, no big deal.

	// for reopened bug:1155118.
	// funcrum has a bug: it can open collections more than 20, but
	// search fails if collections are more than 16. So, if there're
	// more than 16 collections, set delete_searcher TRUE and delete
	// previous searchers.
	if (answerbooks.Count() >= 16)
	    delete_searcher = BOOL_TRUE;

	for (i = 0; i < answerbooks.Count() && delete_searcher == BOOL_FALSE;
				 i++) {
	    if (answerbooks[i]->InitSearcher(err) != STATUS_OK) {
	       delete_searcher = BOOL_TRUE;
	       // Delete any searchers we've already created
	       for (j = 0; j < i; j++)
		   answerbooks[j]->DeleteSearcher();
	       }
	    }

	// Search each AnswerBook in the group in turn.
	//
	for (i = 0; i < answerbooks.Count(); i++) {

		notify->StatusLine(
		      gettext("Searching %s . . ."), ~answerbooks[i]->Title());

		status = answerbooks[i]->Search(*curr_query, results, 
						delete_searcher, err);

		if (status != STATUS_OK) {
			ERRSTK	junk;

			while (err.Depth() > 1)
				err.Pop();

			XvSleep(1, junk);
			notify->Warning(gettext("Search error: %s"), ~err.Pop());
			XvSleep(2, junk);
		}
	}

	if (results.Count() > 0)
	       notify->StatusLine(gettext("Displaying documents found . . ."));


	// Display hit list.
	// If the user has selected the auto-launch-first document
	// in list "execute" the top entry as though the user had
	// double clicked on the first entry in the list.
	// 
	hitlist->Display(results, curr_query->SortOrder(), err);
	if (hitlist->NumHits() > 0  &&  curr_query->FirstDoc())
		MODEMGR::DefaultEventHandler(MM_VIEW_EVENT, (caddr_t)this);


	// Add query to query history.
	// 
	queryhist = GetQueryHist();
	assert(queryhist != NULL);
	queryhist->AddQuery(curr_query->Text());


	notify->Done();


	// Summarize search results in status line.
	//
	if (curr_query->TitlesOnly())
		titlesonly_string = gettext("titles");
	else
		titlesonly_string = gettext("documents");

	if (hitlist->NumHits()) {
		notify->StatusLine(gettext("Found %d %s in Library"),
			hitlist->NumHits(),
			~titlesonly_string);
	} else {
		notify->StatusLine(gettext(
			"Search unsuccessful: no %s found in Library"),
			~titlesonly_string);
	}

	// Record this event for posterity.
	//
	EVENTLOG2("search", "query text = <<%s>>, # hits = %d",
		~curr_query->Text(), hitlist->NumHits());
}

// Handle XView events from SEARCHCTRL buttons.
// Dispatch event handler if one has been registered.
//
void
ButtonEvent(Panel_item button, Event *)
{
	SEARCHMGR	*searchmgr;
	SEARCH_EVENT	event;
	Panel		panel;

	panel = (Panel) xv_get(button, XV_OWNER);
	assert(panel != NULL);
	searchmgr = (SEARCHMGR *) xv_get(panel, XV_KEY_DATA, SEARCHMGR_KEY);
	assert(searchmgr != NULL);
	event = (SEARCH_EVENT) xv_get(button, PANEL_CLIENT_DATA);

	DbgFunc("SEARCHMGR::ButtonEvent: " << event << endl);

	searchmgr->EventHandler(event, (caddr_t)button);
}

// Dispatch events to the event handler.
//
void
SEARCHMGR::DispatchEvent(int event, caddr_t event_obj, caddr_t client_data)
{
	SEARCHMGR	*smgr = (SEARCHMGR *)client_data;

	assert(smgr != NULL);
	smgr->EventHandler(event, event_obj);
}

Notify_value
QueryWinEvent(	Xv_window		win,
		Notify_event		win_event,
		Notify_arg		arg,
		Notify_event_type	type)
{
	SEARCHMGR      *searchmgr;
	SEARCH_EVENT	search_event;
	Notify_value	value;


	value = notify_next_event_func(win, win_event, arg, type);


	// If it is a carriage return, start searching
	if (event_is_down( (Event *)win_event) &&
	    event_is_ascii((Event *)win_event) &&
	    event_id((Event *) win_event) == (char) 0x0d) {

		searchmgr = (SEARCHMGR *) xv_get(win, WIN_CLIENT_DATA);

		assert(searchmgr != NULL);
		DbgFunc("SEARCHMGR::QueryWinEvent (return)" << endl);

		search_event = QUERYWIN_RETURN_EVENT;
		searchmgr->EventHandler(search_event, (caddr_t)NULL);
	}

	return(value);
}

QUERYPROPS *
SEARCHMGR::GetQueryProps()
{
	assert(objstate.IsReady());
	assert(mode_panel != XV_NULL);
	assert(curr_query != NULL);
	assert(abgroup != NULL);
	DbgFunc("SEARCHMGR::GetQueryProps" << endl);


	if (queryprops == NULL) {

		Xv_opaque	parent_frame = xv_get(mode_panel, XV_OWNER);

		// Create query props sheet and register event handler
		// for query props sheet events (i.e., Apply).
		// 
		queryprops = new QUERYPROPS(parent_frame, curr_query, abgroup);
		queryprops->SetEventHandler(DispatchEvent, (caddr_t)this);
	}

	return(queryprops);
}

QUERYHIST *
SEARCHMGR::GetQueryHist()
{
	assert(objstate.IsReady());
	assert(mode_panel != XV_NULL);
	DbgFunc("SEARCHMGR::GetQueryHist" << endl);

	if (queryhist == NULL) {

		Xv_opaque	parent_frame = xv_get(mode_panel, XV_OWNER);

		queryhist = new QUERYHIST(parent_frame);
	}

	return(queryhist);
}
