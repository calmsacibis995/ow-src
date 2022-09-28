#ifndef	_BOOKMARKEDIT_H
#define	_BOOKMARKEDIT_H

#ident "@(#)bookmarkedit.h	1.16 11/15/96 Copyright 1990-1992 Sun Microsystems, Inc."


#include "navigator.h"
#include "xview.h"

//
// Bookmark Editor class.
// BOOKMARKEDIT implements a window for creating and/or editing
// a bookmark's title and comment field.
//


// Forward class references
//
class	NOTIFY;
class	BOOKMARK;
class	INPUTWIN;
class	BOOKMARKEDIT;


// Events generated by BOOKMARKEDIT.
//
typedef enum {
	BMEDIT_SAVE_EVENT	= 1900,	// save newly created bookmark
	BMEDIT_CANCEL_EVENT	= 1901	// cancel creation session
} BMEDIT_EVENT;


class	BOOKMARKEDIT {

    private:

	BOOKMARK	*bookmark;	// the bookmark we're editing/creating

	Frame		frame;
	Panel		panel;

	Panel_item	title_widget;	// title editing field
	Panel_item	save_button;	// Save/create button
	Panel_item	label_widget;	// comment window label
	INPUTWIN	*editwin;	// comment editing window

	NOTIFY		*bmnotify;	// message handler for this window

	// Handle bookmark events.
	//
	void		SaveEvent();	// save edited/new bookmark
	void		CancelEvent();	// cancel editing/creation of bookmark
	void		ResizeEvent();	// adjust widget size/layout

	static void	FrameDoneProc(Frame);
	static void	ButtonEvent(Panel_item, Event *);
	static void	WinEventProc(Panel, Event *);
	static Panel_setting	TitleNotifyProc(Panel_item, Event *);

	// Event handler and accompanying callback argument for
	// bookmark creation events.
	//
	EVENT_HANDLER	event_handler;
	caddr_t		event_client_data;

	// Current state of this object.
	//
	OBJECT_STATE	objstate;


    public:
	
	BOOKMARKEDIT(Xv_opaque parent);
	~BOOKMARKEDIT();

	// Create a new bookmark using the specified bookmark
	// as a starting point.
	//
	void		Edit(BOOKMARK *);

	// We're done creating this bookmark.
	// Clear the contents of the bookmark creation window.
	//
	void		Done();

	// Are we in the process of creating a bookmark?
	//
	BOOL		Busy()		{ return((BOOL)(bookmark != NULL)); }

	// Get the bookmark we're currently editing.
	//
	BOOKMARK	*GetBookmark()	{ return(bookmark); }

	// Display/hide bookmark window.
	//
	void		Show();
	void		Dismiss();

	// Register event handler for BOOKMARKEDIT events.
	//
	void		SetEventHandler(EVENT_HANDLER func, caddr_t clnt)
			    { event_handler = func; event_client_data = clnt; }
};

#endif	_BOOKMARKEDIT_H
