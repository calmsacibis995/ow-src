#ident "@(#)bookmarkmgr.cc	1.83 95/07/25 Copyright 1990-1992 Sun Microsystems, Inc."

#include "bookmarkmgr.h"
#include "bookmarkedit.h"
#include "bookmarklist.h"
#include "inputwin.h"
#include "navigator.h"
#include <doc/abclient.h>
#include <doc/abgroup.h>
#include <doc/bookmark.h>
#include <doc/bookshelf.h>
#include <doc/document.h>
#include <doc/notify.h>
#include <fcntl.h>
#include <locale.h>
#include <sys/stat.h>


static void	ButtonEvent(Panel_item, Event *);

// Detect when user modifies the value in the bookmark title field.
//
Panel_setting	TitleNotifyProc(Panel_item title, Event *event);

// Intercept XView events from Comment Window and pass them on
// to main  "EventHandler()".
//
static Notify_value	CommentEvent(	Xv_window		win,
					Notify_event		event,
					Notify_arg		arg,
					Notify_event_type	type);

BOOKMARKMGR::BOOKMARKMGR(Xv_opaque frame, NOTIFY *notify, ABGROUP *abgrp_arg) :
	MODEMGR			(frame, notify),
	abgroup			(abgrp_arg),
	curr_bookmark		(NULL),
	bookmark_is_modified	(BOOL_FALSE),
	bmlist			(NULL),
	bmwin			(NULL),
	bmnew			(NULL)
{
	int		x, y;
	int		margin_x, margin_y;
	Xv_opaque	win;


	assert(objstate.IsReady());	// should be set in MODEMGR::MODEMGR
	assert(abgroup != NULL);
	assert(mode_panel != NULL);	// ditto
	DbgFunc("BOOKMARKMGR::BOOKMARKMGR" << endl);


	margin_x = 10;
	margin_y = 10;


	// Associate current BOOKMARKMGR object with this panel
	// so we can access it later from event handler.
	//
	xv_set(mode_panel,
			XV_HELP_DATA,		MODE_BOOKMARK_HELP,
			PANEL_LAYOUT,		PANEL_VERTICAL,
			WIN_CLIENT_DATA,	(caddr_t) this,
			NULL);


	// Create new bookmark list.
	//
	x = margin_x;
	y = 0;
	bmlist = new BOOKMARKLIST(mode_panel, x, y);
	xv_set(bmlist->XvHandle(),
			PANEL_LAYOUT,		PANEL_VERTICAL,
			PANEL_LABEL_STRING,   gettext("Bookmarks in Library:"),
			XV_HELP_DATA,		BOOKMARK_LIST_HELP,
			NULL);


	// Register event handler for bookmark list "execute"
	// (double-click) and "select" (single-click) events.
	//
	bmlist->SetEventHandler(BOOKMARKMGR::DispatchEvent, (caddr_t) this);


	// Create bookmark "Delete" button.
	//
	xv_set(mode_panel, PANEL_LAYOUT, PANEL_HORIZONTAL, 0);

	delete_button = xv_create(mode_panel, PANEL_BUTTON,
			PANEL_LABEL_STRING,	gettext("Delete Bookmark"),
			PANEL_NOTIFY_PROC,	ButtonEvent,
			PANEL_CLIENT_DATA,	BM_DELETE_EVENT,
			XV_HELP_DATA,		BOOKMARK_DELETE_BUTTON_HELP,
			NULL);


	// Create bookmark "Save Changes" button.
	//
	save_button = xv_create(mode_panel, PANEL_BUTTON,
			PANEL_LABEL_STRING,	gettext("Save Changes"),
			PANEL_NOTIFY_PROC,	ButtonEvent,
			PANEL_CLIENT_DATA,	BM_SAVE_CHANGES_EVENT,
			PANEL_INACTIVE,		TRUE,
			XV_HELP_DATA,		BOOKMARK_SAVE_BUTTON_HELP,
			NULL);

	xv_set(mode_panel, PANEL_LAYOUT, PANEL_VERTICAL, 0);


	// Create title widget for comment window.
	//
	title_widget = xv_create(mode_panel, PANEL_TEXT,
			XV_X,			margin_x,
			PANEL_LABEL_STRING,	gettext("Comment for:"),
			PANEL_LABEL_BOLD,	TRUE,
			PANEL_VALUE_STORED_LENGTH, 200,
			PANEL_NOTIFY_PROC,	TitleNotifyProc,
			PANEL_NOTIFY_LEVEL,	PANEL_ALL,
			XV_HELP_DATA,		BOOKMARK_TITLE_HELP,
			NULL);

	if (bookshelf->IsReadOnly()) {
		xv_set(title_widget, PANEL_READ_ONLY, TRUE, XV_NULL);
		xv_set(title_widget, PANEL_VALUE_UNDERLINED, FALSE, XV_NULL);
	}


	x = margin_x;
	y = 0;
	
	// Create new bookmark comment window.
	//
	bmwin = new INPUTWIN(frame, x, y, 6);
	bmwin->XvSet(WIN_DESIRED_WIDTH, WIN_EXTEND_TO_EDGE);
	bmwin->XvHelpData(BOOKMARK_COMMENT_HELP);

	if (bookshelf->IsReadOnly()) {
		bmwin->XvSet(TEXTSW_READ_ONLY, TRUE);
	}

	// Set up event handler to detect modifications to the comment window.
	// 
	win = bmwin->XvViewWindow();
	(void) notify_interpose_event_func((Notify_client) win,
					   (Notify_func) CommentEvent,
					   NOTIFY_SAFE);
	xv_set(win, WIN_CLIENT_DATA, (caddr_t)this, XV_NULL);


	// Create "new bookmark" window; register event handler.
	//
	bmnew = new BOOKMARKEDIT(frame);
	bmnew->SetEventHandler(BOOKMARKMGR::DispatchEvent, (caddr_t) this);
}

BOOKMARKMGR::~BOOKMARKMGR()
{
	DbgFunc("BOOKMARKMGR::~BOOKMARKMGR" << endl);

	if (bmlist)
		delete bmlist;
	if (bmwin)
		delete bmwin;
	if (bmnew)
   		delete bmnew;		/* for purify */
}

// Load new list of bookmarks.
//
STATUS
BOOKMARKMGR::Update(ERRSTK &err)
{
	assert(objstate.IsReady());
	DbgFunc("BOOKMARKMGR::LoadBookmarks" << endl);


	// Load bookmarks from bookmark list.
	//
	notify->Busy("");
	bmlist->Load(err);
	notify->Done();


	// If there are no bookmarks,
	// grey out widgets as appropriate.
	//
	// Otherwise, set selection and generate a select event so that
	// the comment window, etc., will get synched up.
	//
	if (bmlist->IsEmpty()) {
		NoBookmarks();
	} else {
		SomeBookmarks();
		bmlist->SetSelection(1);
		SelectEvent();
	}


	// Mission accomplished.
	//
	return(STATUS_OK);
}

void
BOOKMARKMGR::EventHandler(int event, caddr_t event_obj)
{
	BOOKMARK	*bookmark;
	STRING		read_only_msg;
	int		selection;	// currently selected bookmark


	assert(objstate.IsReady());
	assert(bmwin  != NULL);
	assert(bmlist != NULL);
	DbgFunc("BOOKMARKMGR:EventHandler: " << event << endl);

	
	switch (event) {

	case BM_DELETE_EVENT:
		if (bookshelf->IsReadOnly()) {
			read_only_msg =
			    gettext("Can't Delete Bookmark from Read-Only Library");
			notify->Warning( read_only_msg );
		}
		else {
			DeleteEvent();
		}
		break;

	case BM_MODIFIED_EVENT:
		// User edited current bookmark - active "Save Changes" button
		//
		if (( !bookmark_is_modified ) && ( !bmlist->IsEmpty() )) {
			selection = bmlist->GetSelection();
			bookmark = bmlist->GetBookmark( selection );

			if ((bookmark != NULL) &&
			    ( !bmlist->IsHeader( selection )) ) {
				xv_set(save_button, PANEL_INACTIVE, FALSE,
				       XV_NULL);
				bookmark_is_modified = BOOL_TRUE;
			}

		}
		break;

	case BM_SAVE_CHANGES_EVENT:
		SaveChangesEvent();
		break;

	case BMEDIT_SAVE_EVENT:
		SaveEvent();
		break;

	case BMEDIT_CANCEL_EVENT:
		CancelEvent();
		break;

	case WINLIST_SELECT_EVENT:
		SelectEvent();
		break;

	case MM_PANEL_RESIZE_EVENT:	// panel resized: adjust panel widgets
		ResizeEvent();
		break;

	case WINLIST_EXECUTE_EVENT:
	case MM_VIEW_EVENT:
		if (bmlist->NumEntries() > 0) {
		    selection = bmlist->GetSelection();

		    if ((bookmark = bmlist->GetBookmark(selection)) != NULL) {
		      DOCNAME tmpDocname = bookmark->DocName();
		      ABNAME    tmpAbname;
		      tmpAbname.SetABId(tmpDocname.ABId());
		      tmpAbname.SetABVersion(tmpDocname.ABVersion());
		      ABCLIENT* tmpClient;
		      if ((tmpClient = abgroup->GetAnswerBook(tmpAbname)) == NULL) {
			notify->Alert(gettext("Bookmarked AnswerBook not in Library.\nModify Library to include AnswerBook."));
				      break;
		      }
			MODEMGR::DefaultEventHandler(MM_VIEW_EVENT,
						     (caddr_t)this);
		    }
		    else {
		    	notify->Warning(gettext("No document to display"));
		    }
		}
		else {
		    notify->Warning(gettext("No document to display"));
		}
		break;

	default:
		MODEMGR::DefaultEventHandler(event, (caddr_t)this);
		break;
	}

}

void
BOOKMARKMGR::SelectEvent()
{
	BOOKMARK	*bookmark;
	int		selection;	// currently selected bookmark


	assert(objstate.IsReady());
	assert(bmwin  != NULL);
	assert(bmlist != NULL);
	assert(bmlist->NumEntries() > 0);


	// Reset the selection so that it points to a valid bookmark
	// before we proceed.
	//
	selection = bmlist->GetSelection();
	bookmark = bmlist->GetBookmark( selection );


	// If current bookmark has been edited, save the changes,
	// then make the current selection the new "current bookmark".
	//
	SetCurrentBookmark(bookmark, SAVE_CHANGES);


	// If there is no bookmark associated with the current selection,
	// it must be a Book title or a blank line.
 	if ((bookmark != NULL) && ( !bmlist->IsHeader( selection )) ) {
		// Show the annotation and title of the selected bookmark.
		//
		bmwin->SetText(bookmark->Annotation());
		(void) bmwin->TextReadOnly( BOOL_FALSE );
		xv_set(title_widget, PANEL_VALUE, ~bookmark->Title(), XV_NULL);
		xv_set(title_widget, PANEL_INACTIVE, FALSE, NULL);
	}
	else {
		bmwin->ClearText();
		(void) bmwin->TextReadOnly( BOOL_TRUE );
		xv_set(title_widget, PANEL_INACTIVE, TRUE, NULL);
	}
}

void
BOOKMARKMGR::DeleteEvent()
{
	BOOKMARK	*bookmark;
	ERRSTK		err;
	BOOL		found;
	BOOL		really_delete;
	int		selected;
	BOOL		top_of_list;
	STRING		yes_string, no_string;


	assert(objstate.IsReady());
	assert( ! bmlist->IsEmpty());
	DbgFunc("BOOKMARKMGR::DeleteEvent" << endl);
	EVENTLOG("delete bookmark");


	// Find the selected bookmark.
	//
	selected = bmlist->GetSelection();
	bookmark = bmlist->GetBookmark(selected);

	if ((bookmark != NULL) && ( !bmlist->IsHeader( selected ) )) {

		// Find selected bookmark and delete it from the bookmark list.
		//
		yes_string = gettext("Yes");
		no_string  = gettext("No");
		really_delete = notify->AlertPrompt(
					~yes_string,
					~no_string ,
					gettext("Really delete bookmark?"));

		if ( ! really_delete)
			return;


		// Delete the selected bookmark from the bookmark list.
		//
		notify->Busy(gettext("Deleting bookmark . . ."));
		bmlist->DeleteEntry(selected);
		(void) bmlist->Load( err );


		// Forget any outstanding changes to this bookmark.
		//
		bookmark_is_modified = BOOL_FALSE;


		// Re-synch the bookmark list and comment window.
		//
		if (bmlist->IsEmpty()) {
			NoBookmarks();

		} else {
			// Set selection to closest valid bookmark
			// and generate a select event so that
			// the comment window, etc., will get synched up.
			//
			found = BOOL_FALSE;

			if (selected >= bmlist->NumEntries()) {
				selected = bmlist->NumEntries() - 1;
			}

			top_of_list = (BOOL) (selected <= 0);

			while (( !found ) && ( !top_of_list )) {
				bookmark = bmlist->GetBookmark( selected );

				if ((bookmark != NULL) &&
				    ( !bmlist->IsHeader( selected )) ) {
					found = BOOL_TRUE;
				}
				else if (selected <= 1) {
					top_of_list = BOOL_TRUE;
				}
				else {
					selected--;
				}

			};

			if ( top_of_list ) {
				selected = 1;
			}

			bmlist->SetSelection( selected );
			SelectEvent();
		}


		notify->Done();
		notify->StatusLine(gettext("Bookmark deleted"));
	}
	else {
		notify->Warning(gettext("Select a bookmark to delete"));
	}
}

// Create a bookmark to the document currently displayed in the Viewer.
// Add it to user's bookmark list.
//
void
BOOKMARKMGR::CreateBookmark(const DOCNAME &whereami)
{
	BOOKMARK	*new_bookmark;	// bookmark we're creating
	DOCUMENT	*doc;		// document we're marking
	DOCUMENT	*bookroot;	// 
	DOCUMENT	*abroot;	// 
	DOCNAME		tmpname;
	STRING		title;		// title of currently display document
	STRING		abtitle;	// AnswerBook title
	ERRSTK		err;


	assert(objstate.IsReady());
	assert(bmnew != NULL);
	DbgFunc("BOOKMARKMGR::CreateBookmark" << endl);


	// If we're already in the process of creating a bookmark ...
	//
	if (bmnew->Busy()) {
		notify->Alert(gettext("You are already creating a new bookmark"));
		return;
	}


	if ( ! whereami.IsValid()) {
		notify->Warning(gettext(
			"Unable to create bookmark to current page."));
		return;
	}


	// Get document's title.
	// The only way to do this is to actually retrieve the
	// document from the bookdb and look at the title field.
	//
	if ((doc = abgroup->LookUpDoc(whereami, LU_AUTO_ADD, err))  ==  NULL) {
		notify->Alert(
			gettext("Internal error: can't create bookmark."));
		return;
	}


	// Bookmark title is constructed as follows:
	//
	//	<doc title>
	//
	if (doc->Title() != NULL_STRING)
		title = doc->Title();
	else
		title = gettext("[No Title]");

	// Label no longer included in title
//	if (doc->Label() != NULL_STRING)
//		title += " " + doc->Label();

	delete doc;

	MakeBookRootDocName(whereami, tmpname);
	bookroot = abgroup->LookUpDoc(tmpname, LU_AUTO_ADD, err);
	if (bookroot != NULL) {
		abtitle = bookroot->Title();
		delete bookroot;
	}

	MakeAnswerBookRootDocName(whereami, tmpname);
	abroot = abgroup->LookUpDoc(tmpname, LU_AUTO_ADD, err);
	if (abroot != NULL) {
		abtitle += "  (";
		abtitle += abroot->Title();
		abtitle += ")";
		delete abroot;
	}


	EVENTLOG1("new bookmark", "%s", ~title);

	// Create new bookmark object.
	// Initialize fields.
	//
	new_bookmark = new BOOKMARK(whereami);
	new_bookmark->SetTitle(title);
	new_bookmark->SetAnswerBookTitle(abtitle);
	new_bookmark->SetAnnotation(gettext("(none)"));


	// Load bookmark text into bookmark window for editing.
	//
	bmnew->Edit(new_bookmark);
}

void
BOOKMARKMGR::SaveEvent()
{
	BOOKMARK	*bookmark = bmnew->GetBookmark();
	BOOL		end_of_list;
	BOOL		found;
	int		index;
	BOOKMARK	*bookmark_from_list;
	STRING		status;


	assert(objstate.IsReady());
	assert(bmnew != NULL);
	assert(bookmark != NULL);
	DbgFunc("BOOKMARKMGR::SaveEvent" << endl);


	notify->Busy(gettext("Creating new bookmark . . ."));


	// If current bookmark has been edited, save the changes,
	// before we obliterate them with the new bookmark.
	//
	SetCurrentBookmark(bookmark, SAVE_CHANGES);


	// Add bookmark to the end of the list.
	// Highlight the new entry.
	// Have 'SelectEvent()' synch up the comment window.
	//
	if (bmlist->IsEmpty())
		SomeBookmarks();
	bmlist->Add(bookmark);

	found = BOOL_FALSE;
	end_of_list = BOOL_FALSE;
	index = 0;

	while (( !found ) && ( !end_of_list )) {
		bookmark_from_list = bmlist->GetBookmark( index );

		if ((bookmark_from_list == bookmark) &&
		    ( !bmlist->IsHeader( index )) ) {
			found = BOOL_TRUE;
		}
		else if (index < (bmlist->NumEntries() - 1)) {
			index++;
		}
		else {
			end_of_list = BOOL_TRUE;
		}

	};

	if ( end_of_list ) {
		index = bmlist->NumEntries() - 1;
	}

	bmlist->SetSelection( index );
	SelectEvent();

	//XXX need to make sure entry is visible
	if (index > bmlist->GetViewBottom()) {
		bmlist->SetViewBottom( index );
	}
	else if (index < bmlist->GetViewTop()) {
		bmlist->SetViewTop( index );
	}

	bmnew->Done();
	notify->Done();


	status = gettext("Created new bookmark");
	notify->StatusLine(status);
}

void
BOOKMARKMGR::CancelEvent()
{
	BOOKMARK	*bookmark = bmnew->GetBookmark();


	assert(objstate.IsReady());
	assert(bmnew != NULL);
	assert(bookmark != NULL);
	DbgFunc("BOOKMARKMGR::CancelEvent" << endl);


	// Delete new bookmark that we created in 'CreateBookmark()'.
	//
	delete(bookmark);
	bmnew->Done();
}

// Save changes to current bookmark.
//
void
BOOKMARKMGR::SaveChangesEvent()
{
	int	i;
	STRING	indent("      ");


	assert(objstate.IsReady());
	assert(bookmark_is_modified);
	DbgFunc("BOOKMARKMGR::SaveChangesEvent" << endl);

	if (curr_bookmark != NULL) {
		curr_bookmark->SetTitle((char *) xv_get(title_widget,
							PANEL_VALUE));
		curr_bookmark->SetAnnotation(bmwin->GetText());

		bookmark_is_modified = BOOL_FALSE;

		// Update bookmark's title in list
		//
		for (i = 0; i < bmlist->NumEntries(); i++) {
			if ((bmlist->GetBookmark(i) == curr_bookmark) &&
			    ( !bmlist->IsHeader(i) ))
				break;
		}
		assert(i < bmlist->NumEntries());

		bmlist->UpdateEntry(i);
	}
	else {
		notify->Warning(gettext("Select a bookmark to save"));
	}

	xv_set(save_button, PANEL_INACTIVE, TRUE, NULL);
}

void
BOOKMARKMGR::SetCurrentBookmark(BOOKMARK *new_bookmark, CHANGES changes)
{
	STRING	yes_string;
	STRING	no_string;
	STRING	alert_msg;
	BOOL	save_changes;


	assert(objstate.IsReady());
	assert(notify != NULL);
	DbgFunc("BOOKMARKMGR::Show" << endl);


	if (bookmark_is_modified  &&  changes == SAVE_CHANGES) {

		if (curr_bookmark != NULL) {

			yes_string = gettext("Yes, save changes");
			no_string  = gettext("No, discard changes");
			alert_msg = gettext("You have changed the current bookmark.\n Do you want to save those changes?");
			save_changes = notify->AlertPrompt(yes_string,
							   no_string,
							   alert_msg);
			if (save_changes) {
				SaveChangesEvent();
			}
		}

		xv_set(save_button, PANEL_INACTIVE, TRUE, NULL);
		bookmark_is_modified = BOOL_FALSE;
	}


	curr_bookmark = new_bookmark;
}

void
BOOKMARKMGR::Show(BOOL flag)
{
	assert(objstate.IsReady());
	DbgFunc("BOOKMARKMGR::Show" << endl);

	if (flag) {
		xv_set(bmlist->XvHandle(), XV_SHOW, TRUE, NULL);
		ResizeEvent();
		xv_set(mode_panel, XV_SHOW, TRUE, NULL);
		bmwin->Show(BOOL_TRUE);
	} else {
		xv_set(mode_panel, XV_SHOW, FALSE, NULL);
		bmwin->Show(BOOL_FALSE);
		xv_set(bmlist->XvHandle(), XV_SHOW, FALSE, NULL);
	}
}

// Adjust UI to indicate there are no bookmarks in the list.
//
void
BOOKMARKMGR::NoBookmarks()
{
	assert(objstate.IsReady());
	DbgFunc("BOOKMARKMGR::NoBookmarks" << endl);

	xv_set(title_widget,  PANEL_INACTIVE, TRUE, NULL);
	xv_set(delete_button, PANEL_INACTIVE, TRUE, NULL);
	xv_set(save_button,   PANEL_INACTIVE, TRUE, NULL);

	SetCurrentBookmark(NULL, DISCARD_CHANGES);

	bmwin->ClearText();
	xv_set(title_widget, PANEL_VALUE, "", NULL);
	notify->StatusLine( gettext("(No bookmarks)") );
}

// Adjust UI to indicate there are some bookmarks in the list.
//
void
BOOKMARKMGR::SomeBookmarks()
{
	assert(objstate.IsReady());
	DbgFunc("BOOKMARKMGR::SomeBookmarks" << endl);

	xv_set(title_widget,  PANEL_INACTIVE, FALSE, NULL);
	xv_set(delete_button, PANEL_INACTIVE, FALSE, NULL);
}

// Get document id of current selection.
//
const DOCNAME &
BOOKMARKMGR::GetSelection() const
{
	int		selection;
	BOOKMARK	*bookmark;
static	DOCNAME		root_docname;
	

	assert(objstate.IsReady());
	assert(bmlist != NULL);
	DbgFunc("BOOKMARKMGR::GetSelection" << endl);

	selection = bmlist->GetSelection();
	assert(selection >= 0);
	bookmark = bmlist->GetBookmark(selection);

	if ( bmlist->IsHeader( selection ) ) {
		MakeBookRootDocName(bookmark->DocName(), root_docname);
		return( root_docname );
	}
	else {
		return( bookmark->DocName() );
	}
}

void
BOOKMARKMGR::ResizeEvent()
{
	int	margin_x, margin_y;
	int	win_x, win_y;		// annotation window x,y origin
	int	title_x, title_y;	// title widget x,y origin
	int	title_width;		// width of title widget's text field
	int	button_x, button_y;	// button x,y origin
	int	list_width;		// width of bookmark list
	int	list_height;		// height of bookmark list
	int	panel_width;		// width of bookmark panel
	int	panel_height;		// height of bookmark panel


	assert(objstate.IsReady());
	DbgFunc("BOOKMARKMGR::ResizeEvent" << endl);


	margin_x     = 10;
	margin_y     = 10;
	panel_width  = (int) xv_get(mode_panel, XV_WIDTH);
	panel_height = (int) xv_get(mode_panel, XV_HEIGHT);


	// Position annotation window at the bottom of the panel.
	//
	win_x  = margin_x;
	win_y  = panel_height;
	win_y -= (int) xv_get(bmwin->XvHandle(), XV_HEIGHT);
	win_y += (int) xv_get(mode_panel, XV_Y);
	xv_set(bmwin->XvHandle(), XV_X, win_x, XV_Y, win_y, XV_NULL);
	win_y -= (int) xv_get(mode_panel, XV_Y);


	// Position window title widget just above annotation window.
	//
	title_x      = margin_x;
	title_y      = win_y - (int)xv_get(title_widget, XV_HEIGHT) - 5;
	title_width  = panel_width - 25;
	title_width -= (int)xv_get(title_widget, PANEL_VALUE_X);
	xv_set(title_widget, XV_X, title_x, XV_Y, title_y, XV_NULL);
	xv_set(title_widget, PANEL_VALUE_DISPLAY_WIDTH, title_width, XV_NULL);


	// Position buttons a bit above window label.
	//
	button_x  = margin_x;
	button_y  = title_y - (int) xv_get(delete_button, XV_HEIGHT);
	button_y -= 20;
	xv_set(delete_button, XV_X, button_x, XV_Y, button_y, XV_NULL);

	button_x += (int) xv_get(delete_button, XV_WIDTH);
	button_x += 20;
	xv_set(save_button, XV_X, button_x, XV_Y, button_y, XV_NULL);


	// Resize bookmark list to fit into remaining room above
	// the buttons.
	//
	list_height  = button_y    - margin_y;
	list_width   = panel_width - margin_x;
	bmlist->Fit(list_width, list_height);

 	// When window shrinks, the order in which we redraw widgets (above)
 	// causes the buttons and labels to get trashed.
 	// So we redraw them here to insure they are properly rendered.
 	//
	panel_paint(mode_panel, PANEL_NO_CLEAR);
}

void
BOOKMARKMGR::DispatchEvent(int event, caddr_t event_obj, caddr_t client_data)
{
	BOOKMARKMGR	*bmmgr = (BOOKMARKMGR *) client_data;

	assert(bmmgr != NULL);
	DbgFunc("BOOKMARKMGR::DispatchEvent: " << event << endl);

	// Just pass event on to main BOOKMARKMGR event handler.
	//
	bmmgr->EventHandler(event, event_obj);
}

// Handle XView events from BOOKMARK buttons.
// Dispatch event handler if one has been registered.
//
void
ButtonEvent(Panel_item button, Event *)
{
	caddr_t		bmmgr;
	BOOKMARK_EVENT	event;
	Xv_opaque	panel;

	panel = (Xv_opaque) xv_get(button, XV_OWNER);
	bmmgr = (caddr_t)   xv_get(panel,  WIN_CLIENT_DATA);
	event = (BOOKMARK_EVENT) xv_get(button, PANEL_CLIENT_DATA);

	BOOKMARKMGR::DispatchEvent(event, NULL, bmmgr);
}

Notify_value
CommentEvent(	Xv_window		win,
		Notify_event		win_event,
		Notify_arg		arg,
		Notify_event_type	type)
{
	caddr_t		bmmgr;
	Notify_value	value;
	Event* 		event = (Event *)win_event;

	value = notify_next_event_func(win, win_event, arg, type);

	// See if user typed a character in the window,
	// i.e., if the window contents have been modified.

	if ((event_is_down(event) && event_is_iso(event)) ||
		(event_action(event) == ACTION_ERASE_CHAR_BACKWARD) ||
		(event_action(event) == ACTION_ERASE_WORD_BACKWARD) ||
		(event_action(event) == ACTION_ERASE_LINE_BACKWARD) ||
		(event_action(event) == ACTION_ERASE_CHAR_FORWARD) ||
		(event_action(event) == ACTION_ERASE_WORD_FORWARD) ||
		(event_action(event) == ACTION_ERASE_LINE_END) ||
		(event_action(event) == ACTION_CUT) ||
		(event_action(event) == ACTION_PASTE)){
		bmmgr = (caddr_t) xv_get(win, WIN_CLIENT_DATA);
		BOOKMARKMGR::DispatchEvent(BM_MODIFIED_EVENT, NULL, bmmgr);
	}

	return(value);
}

Panel_setting
TitleNotifyProc(Panel_item title, Event *)
{
	caddr_t		bmmgr;
	Xv_opaque	panel;

	panel = (Xv_opaque) xv_get(title, XV_OWNER);
	bmmgr = (caddr_t)   xv_get(panel, WIN_CLIENT_DATA);

	BOOKMARKMGR::DispatchEvent(BM_MODIFIED_EVENT, NULL, bmmgr);

	return(PANEL_INSERT);
}
