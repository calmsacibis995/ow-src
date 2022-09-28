#ifndef	_ABBROWSER_H
#define	_ABBROWSER_H

#ident "@(#)abbrowser.h	1.12 11/15/96 Copyright 1992 Sun Microsystems, Inc."


#include "navigator.h"
#include "xview.h"
#include <doc/abname.h>
#include <doc/list.h>

// Forward references.
//
class	ABINFO;
class	ABGROUP;
class	NOTIFY;
class	WINLIST;

typedef enum {
	BROWSER_APPLY_EVENT	= 200,
	BROWSER_RESET_EVENT	= 201,
	BROWSER_RESIZE_EVENT	= 202
} BROWSER_EVENT;


// AnswerBook Browser.
// Displays list of available AnswerBooks (obtained from ABINFO)
// and allows user to select one or more AnswerBooks for subsequent
// Browsing and Searching operations.
//
class	ABBROWSER {

    private:

	// Window frame, panel for browser.
	//
	Xv_opaque	frame;
	Xv_opaque	panel;
	Xv_opaque	apply_button;
	Xv_opaque	reset_button;

	static Xv_opaque	checkmark_glyph;
	static Xv_opaque	blank_glyph;
	static Xv_opaque	label_glyph;
	static Xv_opaque	label_text;

	// AnswerBook group.
	//
	ABGROUP		&abgroup;

	// List of AnswerBooks.
	//
	WINLIST		*winlist;

	// List of AnswerBook names.
	//
	LIST<ABNAME>	ablist;

	// Error message mechanism.
	//
	NOTIFY		*notify;

	// Current state of this object.
	//
	OBJECT_STATE	objstate;

	// Event handler registered by client via 'SetEventHandler()'.
	//
	EVENT_HANDLER	event_handler;
	caddr_t		event_client_data;

	// Resize Browser window - relayout widgets as needed.
	//
	void		Resize();

	// Event dispatcher and handler for Browser events.
	// The dispatcher is a static method that simply passes
	// the event on to the per-object event handler.
	//
	static void	DispatchEvent(	int	event,
					caddr_t	event_obj,
					caddr_t	client_data);
	void		EventHandler(	int	event,
					caddr_t	event_obj);

	// Apply/Reset button event handler.
	//
	static void	ButtonEvent(Panel_item, Event *);

	// Browser panel event handler.
	//
	static Notify_value	PanelEvent(	Panel,
						Event *,
						Notify_arg,
						Notify_event_type);


    public:

	// Browser constructor, destructor.
	//
	ABBROWSER(Xv_opaque parent, ABGROUP &abgroup_arg);
	~ABBROWSER();

	// Get list of currently selected AnswerBooks.
	// Returns number of AnswerBooks selected.
	//
	void		GetSelection(LIST<ABNAME> &answerbooks);

	// Highlight the Browser entries that are in the current ABGROUP.
	//
	void		Update();

	// Display Browser window.
	//
	void		Show();

	// Dismiss Browser window.
	//
	void		Dismiss();

	// Register handler for Browser events.
	//
	void		SetEventHandler(EVENT_HANDLER func, caddr_t clnt)
			{ event_handler = func; event_client_data = clnt; }
};

#endif	_ABBROWSER_H
