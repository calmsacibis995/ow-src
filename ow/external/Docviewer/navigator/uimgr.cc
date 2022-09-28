#ident "@(#)uimgr.cc	1.62 96/11/15 Copyright 1990-1992 Sun Microsystems, Inc."


#include "uimgr.h"
#include "tocmgr.h"
#include "searchmgr.h"
#include "bookmarkmgr.h"
#include "abbrowser.h"
#include "doc/tt_view_driver.h"
#include <doc/abclient.h>
#include <doc/bookshelf.h>
#include <doc/cardcats.h>
#include <doc/document.h>
#include <doc/dvlink.h>
#include <doc/itimer.h>
#include <doc/notify.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>


UIMGR::UIMGR(Xv_opaque frame, CARDCATS &card_catalogs) :
	abgroup		(card_catalogs),
	toc_mgr		(NULL),
	bookmark_mgr	(NULL),
	search_mgr	(NULL),
	curr_mode_mgr	(NULL),
	abbrowser	(NULL),
	itimer		(NULL),
	main_frame	(frame),
	notify		(main_frame),
	tt_state	(TT_STATE_IDLE),
	tt_pending_op	(NO_OP_PENDING),
	curr_mode	(MODE_NONE),
	xview_argc	(0),
	xview_argv	(NULL),
	navigator_argc	(0),
	navigator_argv	(NULL)
{
	DbgFunc("UIMGR::UIMGR" << endl);


	// We want to interpose on WIN_MAP_NOTIFY events
	// (see "UIMGR::CheckLibrary()").
	//
	xv_set(main_frame, WIN_CLIENT_DATA, (caddr_t)this, XV_NULL);
	xv_set(main_frame,
		WIN_EVENT_PROC,		FrameEventProc,
		WIN_CONSUME_EVENTS,
			ACTION_PROPS,
			XV_NULL,
		XV_NULL);

	// Label window frame if not user defined
	//
	if ((char *) xv_get(main_frame, FRAME_LABEL) == NULL)
		notify.Title(gettext("AnswerBook Navigator v%s"), 
					~navigator_version);

	// Ready to roll...
	//
	objstate.MarkReady();
}

UIMGR::~UIMGR()
{
	DbgFunc("UIMGR::UIMGR" << endl);

	delete toc_mgr;
	delete bookmark_mgr;
	delete search_mgr;
	delete abbrowser;
	if (navigator_argv)
   		delete navigator_argv;
	if (xview_argv)
   		delete xview_argv;
}

static STATUS
_ReallyInit(UIMGR &temp)
{
  return temp.ReallyInit();
}

// Initialize the UI manager.
//
STATUS
UIMGR::Init(ERRSTK &err)
{
	assert(objstate.IsReady());
	DbgFunc("UIMGR::Init" << endl);

	itimer = new ITIMER();
	itimer->TimeOut(1, (TimerCallBack)_ReallyInit, (caddr_t) this);
	notify.Busy("");	// matching "notify.Done()" in ReallyInit()


	// Bring up the TOC mode window.
	//
	assert(curr_mode == MODE_NONE);
	if (SetNavMode(MODE_TOC, err) != STATUS_OK) {
		xv_destroy_safe(main_frame);
		return(STATUS_FAILED);
	}


	return(STATUS_OK);
}

// Save XView-only related run-time arguments
//
void
UIMGR::SaveXViewArgs(int argc, char **argv)
{
	int	index;

	assert(objstate.IsReady());
	DbgFunc("UIMGR::SaveArgs" << endl);

	if ((argc > 0) && (argv != NULL)) {
		xview_argc = argc;
		xview_argv = new char *[xview_argc];

		for (index = 0;  index < xview_argc;  index++) {
			xview_argv[ index ] = argv[ index ];
		}

	}

}

// Save Navigator run-time arguments.
//
// Note that, unlike SaveXViewArgs() above, we don't have to
// allocate new space for args `cause that's already done as the first
// task in the Navigator Init(), routine.
//
void
UIMGR::SaveArgs(int argc, char **argv)
{
	navigator_argc = argc;
	navigator_argv = argv;
}

// Get Navigator run-time arguments.
void
UIMGR::GetArgs(int *argc, char ***argv)
{
	*argc = navigator_argc;
	*argv = navigator_argv;
}

// *Really* initialize the UI manager.
//
STATUS
UIMGR::ReallyInit(void)
{
	TT_VIEW_DRIVER	*ttmgr;
	CARDCATS	&cardcats = abgroup.GetCardCatalogs();
	STRING		alert_msg;
	LISTX<ABINFO*>	ablist;
	LIST<STRING>	ccpaths;
	ERRSTK		err;


	assert(objstate.IsReady());
	assert(itimer != NULL);
	DbgFunc("UIMGR::ReallyInit" << endl);

	delete itimer;	// we're done with it


	// Check the environment - make sure everything is kosher
	// before we begin.

	// Make sure there's at least one card catalog.
	//
	cardcats.GetPaths(ccpaths);
	if (ccpaths.Count() == 0) {

	    alert_msg = gettext("No Card Catalog files were found.\nAnswerBook Navigator cannot locate\n AnswerBooks without a Card Catalog file.\n\nPlease contact your administrator for assistance.\n");

	    notify.Alert(alert_msg);
	    xv_destroy_safe(main_frame);
	    return(STATUS_FAILED);
	}

	// Make sure there's at least one AnswerBook entry
	// in the card catalog(s).
	//
	if (cardcats.GetAll(ablist,err) != STATUS_OK || ablist.Count() == 0) {

	    alert_msg = gettext("No AnswerBooks are listed in your Card Catalog file(s).\nAnswerBook Navigator cannot locate AnswerBooks\nthat are not listed in a Card Catalog file.\n\nPlease contact your administrator for assistance.\n");
	    notify.Alert(alert_msg);
	    xv_destroy_safe(main_frame);
	    return(STATUS_FAILED);
	}


	// Load the specified bookshelf into the Navigator.
	//
	if (LoadBookshelf(bookshelf_path, err)  !=  STATUS_OK  ||
	    NewBookshelf(err)                   !=  STATUS_OK) {
		xv_destroy_safe(main_frame);
		return(STATUS_FAILED);
	}


	// If there are no Bookshelves in the current library,
	// suggest ways the user might find some.
	//
	CheckCurrentLibrary();


	// Register our event handler with the ToolTalk manager.
	//
	assert(navigator != NULL);
	ttmgr = navigator->GetTTMgr();
	assert(ttmgr != NULL);
	ttmgr->SetEventHandler(UIMGR::DispatchEvent, (caddr_t)this);


	notify.Done();		// matches "notify.Busy()" in Init()
	return(STATUS_OK);
}

// Set the preferred viewing language for this session.
//
void
UIMGR::SetPreferredLanguage(const STRING &lang)
{
	assert(objstate.IsReady());
	DbgFunc("UIMGR::SetPreferredLanguage: " << lang << endl);

	abgroup.SetPreferredLang(lang);
}

// A new Bookshelf has just been loaded into the Navigator.
// Update the various mode windows (TOC, Search, Bookmark) as necessary.
//
STATUS
UIMGR::NewBookshelf(ERRSTK &err)
{
	STRING	mode_msg;
	int	i;


	assert(objstate.IsReady());
	assert(bookshelf != NULL);
	DbgFunc("UIMGR::NewBookshelf" << endl);


	// Clear out existing AnswerBooks.
	//
	abgroup.answerbooks.Clear();


	// Add AnswerBooks in bookshelf to abgroup.
	//
	LIST<ABNAME>	&abnames = bookshelf->answerbooks;
	for (i = 0; i < abnames.Count(); i++) {
		if (abgroup.AddAnswerBook(abnames[i], err) != STATUS_OK) {
			notify.ShowErrs(err);
		}
	}


	// (Re)initialize the various mode windows as necessary.
	//
	if (toc_mgr != NULL)
		toc_mgr->Update(err);
	if (search_mgr != NULL)
		search_mgr->Update(err);
	if (bookmark_mgr != NULL)
		bookmark_mgr->Update(err);


	// Set book title in the footer.
	//
	if (bookshelf->IsReadOnly())
		mode_msg = gettext("Library: %s (read-only)");
	else
		mode_msg = gettext("Library: %s");

	notify.Mode(mode_msg, ~bookshelf->GetPath());


	return(STATUS_OK);
}

// Load new bookshelf into Navigator.
//
STATUS
UIMGR::LoadBookshelf(const STRING &bspath, ERRSTK &err)
{
	BOOKSHELF	*bs;
	STRING		alert_msg;
	STRING		quit_msg;
	STRING		unlock_msg;
	STRING		rdonly_msg;
	int		flags;
	BOOL		bs_exists = BOOL_FALSE;
	BOOL		bs_locked = BOOL_FALSE;
	BOOL		bs_rdonly = BOOL_TRUE;
	BOOL		reply;


	assert(objstate.IsReady());
	assert(bspath != NULL_STRING);
	DbgFunc("UIMGR::LoadBookshelf: " << bspath << endl);


	// Find out about this bookshelf file.
	//
	if (access(bspath, F_OK)  ==  0) {

		bs_exists = BOOL_TRUE;

		if (access(bspath, W_OK)  ==  0)
			bs_rdonly = BOOL_FALSE;

		if (BOOKSHELF::FileIsLocked(bspath))
			bs_locked = BOOL_TRUE;
	}


	// If bookshelf doesn't exist, create it.
	//
	if ( ! bs_exists) {

		flags = BS_RDWR|BS_CREAT|BS_SET_LOCK;

	} else if (bs_rdonly) {

		alert_msg  = gettext("The AnswerBook Library is read only.\nThe Library filename is ");
		alert_msg += bspath;
		alert_msg += gettext(".\nYou may open a Navigator, but your changes will not be saved.");

		rdonly_msg = gettext("Open Read Only");
		quit_msg   = gettext("Quit");

		reply = notify.AlertPrompt(rdonly_msg, quit_msg, alert_msg);
		if (reply) {
			flags = BS_RDONLY;
		} else {
			return(STATUS_FAILED);
		}

	} else if (bs_locked) {

		alert_msg    = gettext("The AnswerBook Library is locked  for use by another Navigator.\n The Library filename is ");
		alert_msg += bspath;
		alert_msg   += gettext(".\nYou may continue and open a Navigator using the locked\nLibrary file for read only, but changes will not be saved.\n\nIf you unlock the Library file and make changes in this Navigator,\nyou may lose settings and bookmarks from another Navigator.\n");
		rdonly_msg = gettext("Open Read Only");
		unlock_msg = gettext("Reset Lock, then Open");

		reply = notify.AlertPrompt(rdonly_msg,unlock_msg,alert_msg);

		if (reply) {
			flags = BS_RDONLY;
		} else {
			flags = BS_RDWR|BS_RESET_LOCK;
		}

	} else {
		flags = BS_RDWR|BS_SET_LOCK;
	}


	if ((bs = BOOKSHELF::OpenFile(bspath, flags, err, notify))  ==  NULL) {
		notify.ShowErrs(err);
		return(STATUS_FAILED);
	}


	// Now get the AnswerBook list, bookmark list, etc.,
	// from the Library file.
	//
	if (bs->ReadFile(err)  !=  STATUS_OK) {
		delete bs;
		return(STATUS_FAILED);
	}


	// Delete the old bookshelf, if any.
	//
	if (bookshelf != NULL)
		delete bookshelf;
	bookshelf = bs;


	return(STATUS_OK);
}

// Check to see whether the current library is empty,
// and if so, tell the user how to remedy the situation.
//
void
UIMGR::CheckCurrentLibrary()
{
	STRING	alert_msg;


	assert(objstate.IsReady());
	DbgFunc("UIMGR::CheckCurrentLibrary" << endl);


	// If the current bookshelf isn't empty, we're ok.
	//
	if (abgroup.answerbooks.Count() > 0)
		return;


	// Suggest to user how they can resolve the current situation.
	//
	alert_msg = gettext("There are no AnswerBooks in the AnswerBook Library\n");
	alert_msg += ~bookshelf->GetPath();
	alert_msg  += gettext(".\n\nClick SELECT on the 'Modify Library . . .' button\nfor a list of available AnswerBooks");

	notify.Alert(alert_msg);
}

// Dispatch various UI events.
//
void
UIMGR::EventHandler(int event, caddr_t event_obj)
{
	ABBROWSER	*browser;
	DOCNAME		docname;
	STRING		bspath;		// bookshelf path name
	STRING		read_only_msg;
	LIST<ABNAME>	ablist;		// list of AnswerBooks from Browser
	TT_VIEW_DRIVER		*ttmgr;
	TT_VIEW_DRIVER_STATUS	*ttstatus;	// tooltalk response
	DOCNAME		*whereami;
	int		i;
	ERRSTK		err;
	BOOL		viewer_responded;
	STRING		alert_msg;


	DbgFunc("UIMGR::EventHandler: " << event << endl);


	switch (event) {

	case MM_MODE_SELECT_EVENT:
		assert(curr_mode_mgr != NULL);
		SetNavMode(curr_mode_mgr->GetMode(), err);
		EVENTLOG("select mode");
		break;

	case MM_VIEW_EVENT:
		assert(curr_mode_mgr != NULL);
		docname = curr_mode_mgr->GetSelection();
		ViewDocument(docname);
		EVENTLOG("view document");
		break;

	case MM_BOOKMARK_EVENT:
		if (bookshelf->IsReadOnly()) {
			read_only_msg =
			    gettext("Can't Place Bookmark in Read-Only Library");
			notify.Warning( read_only_msg );
			break;
		}
		assert(navigator != NULL);
		ttmgr = navigator->GetTTMgr();
		assert(ttmgr != NULL);
		if ( ! ttmgr->IsViewerPresent()) {
			notify.Warning(gettext(
				"You must be viewing a document"
				" to create a new bookmark."));
			break;
		}
		notify.Busy("");
		if (ttmgr->SendWhereAmIMsg(err)  !=  STATUS_OK) {
			err.Init(gettext("System Error.\nPlease Quit and Start a New Navigator."));
			notify.ShowErrs(err);
			notify.Done();
			break;
		}
		assert(tt_state == TT_STATE_IDLE);
		tt_state = WAITING_FOR_WHEREAMI_REPLY;
		assert(tt_pending_op == NO_OP_PENDING);
		tt_pending_op = CREATE_BOOKMARK_OP_PENDING;
		EVENTLOG("new bookmark");
		break;

	case MM_BROWSE_EVENT:
		browser = GetAnswerBookBrowser(err);
		assert(browser != NULL);
		browser->Update();
		browser->Show();
		EVENTLOG("show answerbook browser");
		break;

	case BROWSER_APPLY_EVENT:
		if (bookshelf->IsReadOnly()) {
			alert_msg = gettext("Applying changes for this session only\nCan't save in Read-Only Library.");
			notify.Alert( alert_msg );
		}

		browser = (ABBROWSER *)event_obj;
		assert(browser != NULL);
		browser->GetSelection(ablist);
		assert(bookshelf != NULL);

		// Clear current list of AnswerBooks in "abgroup"
		// and in current bookshelf.
		//
		abgroup.answerbooks.Clear();
		bookshelf->answerbooks.Clear();

		// Add new AnswerBooks to "abgroup" and current bookshelf.
		// Mark current bookshelf as having been modified.
		//
		for (i = 0; i < ablist.Count(); i++) {
		    if (abgroup.AddAnswerBook(ablist[i], err) != STATUS_OK) {
			notify.ShowErrs(err);
		    } else {
			bookshelf->answerbooks.Add(ablist[i]);
		    }
		}

		bookshelf->MarkDirty();

		// Update Table of Contents window and Search window
		// to reflect new list of AnswerBooks.
		//
		if (toc_mgr != NULL  &&
		    toc_mgr->Update(err)  !=  STATUS_OK) {
			notify.ShowErrs(err);
			browser->Update();
			break;
		}
		if (search_mgr != NULL  &&
		    search_mgr->Update(err)  !=  STATUS_OK) {
			notify.ShowErrs(err);
			browser->Update();
			break;
		}

		// Update selection in AnswerBook browser window.
		//
		browser->Dismiss();
		browser->Update();
		EVENTLOG("apply browser list");
		break;

	case BROWSER_RESET_EVENT:
		browser = (ABBROWSER *)event_obj;
		assert(browser != NULL);
		browser->Update();
		EVENTLOG("reset browser list");
		break;

	case TT_VIEW_DRIVER_VIEW_DOCUMENT_REPLY:
		assert(tt_state == WAITING_FOR_VIEW_DOCUMENT_REPLY);
		assert(tt_pending_op == NO_OP_PENDING);
		tt_state = TT_STATE_IDLE;
		ttstatus = (TT_VIEW_DRIVER_STATUS*)event_obj;
		assert(ttstatus != NULL);
		notify.Done();

		if (ttstatus->status != STATUS_OK) {
		    viewer_responded = * (BOOL *) ttstatus->tt_arg;

		    if ( viewer_responded ) {
			ttstatus->err.Push(gettext("Can't view document."));
			notify.ShowErrs(ttstatus->err);
		    }
		    else {
			docname = curr_mode_mgr->GetSelection();
			ViewDocument(docname);
		    }
		    break;
		}
		assert(navigator != NULL);
		ttmgr = navigator->GetTTMgr();
		assert(ttmgr != NULL);
		assert(ttmgr->IsViewerPresent());
		(void) ttmgr->SetIconified(BOOL_FALSE, err);
/*
		(void) ttmgr->Raise( err );
*/
		break;

	case TT_VIEW_DRIVER_WHEREAMI_REPLY:
		assert(tt_state == WAITING_FOR_WHEREAMI_REPLY);
		ttstatus = (TT_VIEW_DRIVER_STATUS*)event_obj;
		assert(ttstatus != NULL);
		whereami = (DOCNAME *)ttstatus->tt_arg;
		notify.Done();

		switch(tt_pending_op) {
		case CREATE_BOOKMARK_OP_PENDING:
			if (ttstatus->status != STATUS_OK) {
				ttstatus->err.Push(
					gettext("Can't create bookmark."));
				notify.ShowErrs(ttstatus->err);
			} else {
				assert(whereami != NULL);
				bookmark_mgr = GetBookmarkMgr(err);
				bookmark_mgr->CreateBookmark(*whereami);
			}
			break;
		default:
			assert(0);
		}
		tt_state = TT_STATE_IDLE;
		tt_pending_op = NO_OP_PENDING;
		break;

	case TT_VIEW_DRIVER_VIEWER_STARTUP_REPLY:
		assert(tt_state == WAITING_FOR_VIEWER_STARTUP_REPLY);
		assert(curr_mode_mgr != NULL);
		ttstatus = (TT_VIEW_DRIVER_STATUS*)event_obj;
		assert(ttstatus != NULL);
		if (ttstatus->status != STATUS_OK) {
			ttstatus->err.Push(gettext("Can't start new Viewer."));
			notify.Done();
			notify.ShowErrs(ttstatus->err);
			break;
		}

		notify.Done();
		assert(navigator != NULL);
		ttmgr = navigator->GetTTMgr();
		assert(ttmgr != NULL);
		assert(ttmgr->IsViewerPresent());
		(void) ttmgr->SetIconified(BOOL_FALSE, err);
		(void) ttmgr->Raise( err );
		tt_state = TT_STATE_IDLE;
		tt_pending_op = NO_OP_PENDING;
		break;

	case TT_VIEW_DRIVER_VIEWER_EXIT_MSG:
		assert(navigator != NULL);
		ttmgr = navigator->GetTTMgr();
		assert(ttmgr != NULL);
		assert( ! ttmgr->IsViewerPresent());
		notify.Done();

		// Exit navigator.
		//
//XXX		xv_destroy_safe(main_frame);
		break;

	default:
		cerr << "UIMGR::EventHandler: unknown event: " << event <<endl;
		break;
	}
}

// Pass UI events on to the *real* UIMGR event handler.
//
void
UIMGR::DispatchEvent(int event, caddr_t event_obj, caddr_t client_data)
{
	UIMGR	*uimgr = (UIMGR *)client_data;

	assert(uimgr != NULL);
	uimgr->EventHandler(event, event_obj);
}

void
UIMGR::FrameEventProc(const Xv_window		frame,
		      Event *const		event,
		      const Notify_arg		/* unused */)
{
	UIMGR	*uimgr;
	
	DbgFunc("UIMGR::FrameEventProc:\n" << event << endl);

	switch (event_action(event)) {

	case ACTION_PROPS:
		uimgr = (UIMGR *)xv_get(frame, WIN_CLIENT_DATA);
		assert(uimgr != NULL);

		if (uimgr->curr_mode == MODE_SEARCH) {
			uimgr->search_mgr->EventHandler(SEARCH_PROPS_EVENT,
							NULL);
		}
		break;

	default:
		break;
	}

}

// Display file in Viewer window. (start viewer)

STATUS
UIMGR::ViewFile (const STRING &filename, ERRSTK &err)
{
	TT_VIEW_DRIVER	*ttmgr = navigator->GetTTMgr();
	STRING		language;
	STRING		cclist;
	STATUS		status;
	int		x_position,
			y_position;

	assert(navigator != NULL);
	assert(ttmgr != NULL);
	DbgFunc("UIMGR::ViewFile" << endl);

	language = abgroup.PreferredLang();
	abgroup.GetCardCatalogs().GetPaths(cclist);

	notify.Busy(gettext("Starting new Viewer . . ."));
	GetViewerPosition(x_position, y_position);
	
	status = ttmgr->LaunchViewerWithFile (filename, language,
					cclist, x_position, y_position, err);

	if (status != STATUS_OK) {
		notify.Done();
		err.Push(gettext("Could not start new Viewer"));
		return(STATUS_FAILED);
	}


	// Note that we're expecting a reply to this tooltalk message
	// once the Viewer comes up.
	//
	tt_state      = WAITING_FOR_VIEWER_STARTUP_REPLY;


	return(STATUS_OK);
}

// Display document in Viewer window.
// Start new Viewer if necessary.
//
void
UIMGR::ViewDocument(const DOCNAME &docname)
{
	DOCUMENT	*doc;
	STRING		title;		// title of current selection
	DVLINK		view_method;	// view method of current selection
	DOCNAME		tmpname;
	TT_VIEW_DRIVER	*ttmgr;
	u_long		flags;
	ERRSTK		err;


	assert(objstate.IsReady());
	assert(docname.IsValid());


	// Get document's record.
	// Document could be a symbolic link; if so, resolve it before
	// we proceed.  Also, if the a translation of the document is
	// available in the current "preferred language", give us the
	// translation rather than the English version.
	//
	flags = LU_RESOLVE_SYMLINK|LU_PREFERRED_LANG;
	if ((doc = abgroup.LookUpDoc(docname, flags, err))  ==  NULL) {
		err.Push(gettext("Can't view document '%s'"), ~docname);
		notify.ShowErrs(err);
		return;
	}


	// Get document title, view method.
	// Verify that document viewing method is valid.
	// It's ok if certain documents in the TOC don't have
	// view methods (i.e., all documents above the 'book' level).
	// All other documents should have a view method.
	//
	if ((title = doc->Title()) == NULL_STRING)
		title = gettext("(No Title)");
	if (view_method.SetCookie(doc->ViewMethod())  !=  STATUS_OK) {
		if (curr_mode_mgr->GetMode() != MODE_TOC)
			notify.Warning(gettext("Document not viewable: %s"),
					~title);
		delete doc;
		return;
	}



	tmpname = doc->Name();
	delete doc;


	EVENTLOG1("view document", "%s", ~title);
	DbgFunc("UIMGR::ViewDocument: " << title << endl);


	// Send a "View Document" message to the Viewer.
	//
	assert(navigator != NULL);
	ttmgr = navigator->GetTTMgr();
	assert(ttmgr != NULL);
	if (ttmgr->IsViewerPresent()) {

		notify.Busy(gettext("Viewing document '%s' . . ."), ~title);

		if (ttmgr->SendViewDocumentMsg(tmpname, err)  !=  STATUS_OK) {
			notify.Done();
			err.Init(gettext("System Error.\nPlease Quit and Start a New Navigator."));
			notify.ShowErrs(err);
			return;
		}

		// Note that we're expecting a reply to this tooltalk message.
		//
		tt_state = WAITING_FOR_VIEW_DOCUMENT_REPLY;

	} else {
		if (LaunchNewViewer(tmpname, err) != STATUS_OK) {
			notify.ShowErrs(err);
			return;
		}
	}
}

STATUS
UIMGR::LaunchNewViewer(const DOCNAME &docname, ERRSTK &err)
{
	TT_VIEW_DRIVER	*ttmgr = navigator->GetTTMgr();
	STRING		language;
	STRING		cclist;
	STATUS		status;
	int		x_position,
			y_position;


	assert(objstate.IsReady());
	assert(navigator != NULL);
	assert(ttmgr != NULL);
	DbgFunc("UIMGR::LaunchNewViewer" << endl);

	language = abgroup.PreferredLang();
	abgroup.GetCardCatalogs().GetPaths(cclist);

	notify.Busy(gettext("Starting new Viewer . . ."));
	GetViewerPosition(x_position, y_position);

	if ((xview_argc > 0) && (xview_argv != NULL)) {
		status = ttmgr->LaunchViewerWithDocname(docname, language,
					cclist, xview_argc, xview_argv,
					x_position, y_position, err);
	}
	else {
		status = ttmgr->LaunchViewerWithDocname(docname, language,
					cclist, x_position, y_position, err);
	}

	if (status != STATUS_OK) {
		notify.Done();
		err.Push(gettext("Could not start new Viewer"));
		return(STATUS_FAILED);
	}


	// Note that we're expecting a reply to this tooltalk message
	// once the Viewer comes up.
	//
	tt_state      = WAITING_FOR_VIEWER_STARTUP_REPLY;


	return(STATUS_OK);
}

void	UIMGR::GetViewerPosition(int &x_position, int &y_position)
{
	Rect	rect;
	Display *display;
	int screenwidth;
	int midpoint;

	x_position = 380;
	y_position = 0;
	frame_get_rect(main_frame, &rect);
	midpoint = rect.r_left + (rect.r_width / 2);

	display = (Display *) xv_get(main_frame, XV_DISPLAY);
	screenwidth = DisplayWidth(display, DefaultScreen(display));

	if (rect.r_left > 0) {
		if (midpoint  > (screenwidth / 2))
			x_position = 0;
	}
}

// Change navigator mode (Contents, Search, Bookmark).
// If successfull, returns new mode.
// Otherwise, returns current mode.
//
STATUS
UIMGR::SetNavMode(int new_mode, ERRSTK &err)
{
	MODEMGR		*new_mode_mgr = NULL;


	assert(objstate.IsReady());


	if (new_mode == curr_mode)
		return(STATUS_OK);


	switch (new_mode) {

	case MODE_TOC:
		new_mode_mgr = GetTocMgr(err);
		EVENTLOG("toc mode");
		break;

	case MODE_SEARCH:
		new_mode_mgr = GetSearchMgr(err);
		EVENTLOG("search mode");
		break;

	case MODE_BOOKMARK:
		new_mode_mgr = GetBookmarkMgr(err);
		EVENTLOG("bookmark mode");
		break;

	case MODE_NONE:
	default:
		assert(0);
		break;
	}

	if (new_mode_mgr == NULL) {
		if (curr_mode_mgr != NULL  &&  curr_mode != MODE_NONE)
			curr_mode_mgr->SetMode(curr_mode);
		return(STATUS_FAILED);
	}


	// Take down old lists and windows
	// and put up the new ones.
	// Send a resize event to the new mode panel just in case
	// it hasn't gotten one yet (i.e., when the first time).
	// This is redundant, but it's cheap and it gets the job done
	// (previously we were missing initial resize events under some
	// circumstances).
	//
	if (curr_mode_mgr)
		curr_mode_mgr->Show(BOOL_FALSE);
	new_mode_mgr->Show(BOOL_TRUE);
	new_mode_mgr->SetMode(new_mode);

	curr_mode     = new_mode;
	curr_mode_mgr = new_mode_mgr;

	DbgFunc("UIMGR::SetNavMode: " <<  new_mode << endl ) ;
	return(STATUS_OK);
}

// Initialize TOC manager.
//
TOCMGR *
UIMGR::GetTocMgr(ERRSTK &)
{
	assert(objstate.IsReady());
	DbgFunc("UIMGR::GetTocMgr" << endl);

	
	// If this is the first time,
	// create and initialize the toc manager.
	//
	if (toc_mgr == NULL) {
		toc_mgr = new TOCMGR(main_frame, &notify, &abgroup);
		toc_mgr->SetEventHandler(UIMGR::DispatchEvent, (caddr_t)this);
	}

	return(toc_mgr);
}

// Initialize SEARCH manager.
//
SEARCHMGR *
UIMGR::GetSearchMgr(ERRSTK &)
{
	assert(objstate.IsReady());
	DbgFunc("UIMGR::GetSearchMgr" << endl);

	
	// If this is the first time,
	// create and initialize the search manager.
	//
	if (search_mgr == NULL) {
		search_mgr = new SEARCHMGR(main_frame, &notify, &abgroup);
		search_mgr->SetEventHandler(	UIMGR::DispatchEvent,
						(caddr_t)this);
	}

	return(search_mgr);
}

// Initialize BOOKMARK manager.
//
BOOKMARKMGR *
UIMGR::GetBookmarkMgr(ERRSTK &err)
{
	assert(objstate.IsReady());
	DbgFunc("UIMGR::GetBookmarkMgr" << endl);

	
	// If this is the first time,
	// create and initialize the bookmark manager.
	//
	if (bookmark_mgr == NULL) {
		bookmark_mgr = new BOOKMARKMGR(main_frame, &notify, &abgroup);
		bookmark_mgr->SetEventHandler(	UIMGR::DispatchEvent,
						(caddr_t)this);
		if (bookmark_mgr->Update(err)  !=  STATUS_OK) {
			delete bookmark_mgr;
			bookmark_mgr = NULL;
		}
		else {
			bookmark_mgr->ResizeEvent();
		}
	}

	return(bookmark_mgr);
}

// Initialize AnswerBook browser.
//
ABBROWSER *
UIMGR::GetAnswerBookBrowser(ERRSTK &/*err*/)
{
	assert(objstate.IsReady());
	DbgFunc("UIMGR::GetAnswerBookBrowser" << endl);

	
	// If this is the first time,
	// create and initialize the toc manager.
	//
	if (abbrowser == NULL) {

		abbrowser = new ABBROWSER(main_frame, abgroup);
		abbrowser->SetEventHandler(	UIMGR::DispatchEvent,
						(caddr_t)this);
	}

	return(abbrowser);
}

