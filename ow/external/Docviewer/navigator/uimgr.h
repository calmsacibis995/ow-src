/* RE_SID: @(%)/export/build0/source/SOURCE+SCCS_S297_FCS/external/Docviewer/navigator/SCCS/s.uimgr.h 1.25 96/11/15 15:27:11 SMI */
#ifndef	_UIMGR_H
#define	_UIMGR_H

#ident "@(#)uimgr.h	1.25 96/11/15 Copyright 1990-1992 Sun Microsystems, Inc."

#include "navigator.h"
#include "xview.h"
#include <doc/abgroup.h>
#include <doc/docname.h>
#include <doc/notify.h>
#include <xview/notify.h>
#include <time.h>

// Forward references.
//
class	ABBROWSER;
class	BOOKMARKMGR;
class	BOOKSHELF;
class	CARDCATS;
#ifdef	USER_COMMENTS
class	COMMENTS;
#endif	USER_COMMENTS
class	ITIMER;
class	MODEMGR;
class	SEARCHMGR;
class	TOCMGR;
class	TTMGR;


// ToolTalk presents us with an asynchronous call/reply interface to
// the Viewer.  The context in which a ToolTalk message is sent
// influences how we handle the reply.  So we use TT_STATE to keep track
// of that context.  Valid replies are:
//
//   TT_STATE_IDLE			We're idle - not waiting for anything
//
//   WAITING_FOR_WHEREAMI_REPLY	Waiting for Viewer to tell us which
//					document it's currently displaying
//
//   WAITING_FOR_VIEW_DOCUMENT_REPLY	Waiting for Viewer to finish
//					displaying a document
//
//   WAITING_FOR_VIEWER_STARTUP_REPLY	Waiting for new Viewer to come up
//
enum TT_STATE {
	TT_STATE_IDLE,
	WAITING_FOR_WHEREAMI_REPLY,
	WAITING_FOR_VIEW_DOCUMENT_REPLY,
	WAITING_FOR_VIEWER_STARTUP_REPLY
};

// In addition, one of several operations can be queued pending receipt
// of a ToolTalk message reply.  We use TT_PENDING_OP to track this info.
//
enum TT_PENDING_OP {
	NO_OP_PENDING,
	CREATE_BOOKMARK_OP_PENDING,
	VIEW_DOCUMENT_OP_PENDING
};


// XXX
//
class	UIMGR {

    private:

	MODEMGR		*curr_mode_mgr;
	int		curr_mode;

	// Table of Contents manager.
	// This object manages all aspects of toc's,
	// including the toc UI and interfacing to the LDA.
	//
	TOCMGR		*toc_mgr;

	// Search manager.
	// This object manages all aspects of searching,
	// including the search UI and interfacing to the search engine.
	//
	SEARCHMGR	*search_mgr;

	// Bookmark manager.
	// This object manages all aspects of bookmarking,
	// including bookmark creation, editing, and following.
	//
	BOOKMARKMGR	*bookmark_mgr;

	// AnswerBook browser.
	//
	ABBROWSER	*abbrowser;

	// "About the Navigator" popup window.
	//


	// Navigator window frame.
	//
	Xv_opaque	main_frame;

	// Presenter of error/warning/status messages.
	//
	NOTIFY		notify;

	// Current AnswerBook Group we're navigating.
	//
	ABGROUP		abgroup;

	// Interval timer used during startup.
	//
	ITIMER		*itimer;

	// ToolTalk presents us with an asynchronous call/reply interface
	// to the Viewer.  The context in which a ToolTalk message is sent
	// influences how we handle the reply.  So we use TT_STATE to keep
	// track of that context.
	//
	TT_STATE	tt_state;

	// In addition, one of several operations can be queued pending
	// receipt of a ToolTalk message reply.  We use TT_PENDING_OP
	// to track this info.
	//
	TT_PENDING_OP	tt_pending_op;

	// When Viewer is first launched, name of document to display
	// once it comes up.
	//
	DOCNAME		pending_document;

	// run-time args to be passed to windoe manager upon
	// a "Save Workspace".
	int		navigator_argc;
	char		**navigator_argv;

	// XView-only run-time args to be passed on the the viewer
	int		xview_argc;
	char		**xview_argv;

	// Current state of this object.
	//
	OBJECT_STATE	objstate;

	// Access various Navigator components.
	// Because these components tend to be transient (they get
	// created and deleted on the fly), all access to them within
	// UIMGR should be through these routines.
	//
	TOCMGR		*GetTocMgr(ERRSTK &);
	SEARCHMGR	*GetSearchMgr(ERRSTK &);
	BOOKMARKMGR	*GetBookmarkMgr(ERRSTK &);
	ABBROWSER	*GetAnswerBookBrowser(ERRSTK &);

	// Event dispatcher and handler for Navigator UI events.
	// The dispatcher is a static method that simply passes
	// the event on to the per-object event handler.
	//
	static void	DispatchEvent(	int	event,
					caddr_t	event_obj,
					caddr_t	client_data);
	void		EventHandler(	int	event,
					caddr_t	event_obj);

	// Handler for window frame events.
	//
	static Notify_value	FrameEvent(	Xv_opaque	frame,
						Event		*event,
						Notify_arg	arg,
						Notify_event_type type);

	static void	FrameEventProc(const Xv_window		win,
				       Event *const		ev,
				       const Notify_arg		/* unused */);

	// View specified document.
	//
	void		ViewDocument(const DOCNAME &docname);

	// Launch new Viewer for this Navigator.
	//
	STATUS		LaunchNewViewer(const DOCNAME &docname, ERRSTK &);

	// Get appropriate position for Viewer
	//
	void		GetViewerPosition(int	&x_position, int &y_position);

	// This is where the UIMGR *really* gets initialized.
	//

	// Load specified bookshelf into Navigator.
	//
	STATUS		LoadBookshelf(const STRING &bs_path, ERRSTK &err);
	STATUS		NewBookshelf(ERRSTK &err);

	// Check to see whether the current library is empty,
	// and if so, tell the user how to remedy the situation.
	//
	void		CheckCurrentLibrary();


    public:
	
	// UIMGR constuctor, destructor.
	//
	UIMGR(Xv_opaque frame, CARDCATS &card_catalogs);
	~UIMGR();

	STATUS		ReallyInit(void);
	// Initialize this UIMGR.
	//
	STATUS		Init(ERRSTK &err);

	// Save run-time arguments to be passed to window manager
	// upon "Save Workspace".
	//
	void		SaveArgs(int argc, char **argv);

	// Get run-time arguments passed to the Navigator.
	//
	void		GetArgs(int *argc, char ***argv);

	// Save XView-only run-time arguments to be passed to viewer
	//
	void		SaveXViewArgs(int argc, char **argv);

	// Return XView handle to main window frame
	//
	Xv_opaque	FrameHandle() const
	{
		assert(objstate.IsReady());
		assert(main_frame);
		return(main_frame);
	}

	// Set the preferred viewing language for this session.
	//
	void		SetPreferredLanguage(const STRING &lang);
	
	// Launch new Viewer for this file
	STATUS		ViewFile (const STRING &filename,  ERRSTK &err);

	// Set the current Navigation Mode (search, toc browse, bookmark).
	//
	STATUS		SetNavMode(int mode, ERRSTK &err);
	int		GetNavMode() const	{ return(curr_mode); }

	MODEMGR		*GetModeMgr( void ) { return( curr_mode_mgr ); }
};

#endif	_UIMGR_H
