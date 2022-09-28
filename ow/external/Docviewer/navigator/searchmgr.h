#ifndef	_SEARCHMGR_H
#define	_SEARCHMGR_H

#ident "@(#)searchmgr.h	1.20 06/11/93 Copyright 1990-1992 Sun Microsystems, Inc."


#include "modemgr.h"


// Forward class declarations.
//
class	HITLIST;
class	INPUTWIN;
class	QUERY;
class	QUERYHIST;
class	QUERYPROPS;


// SEARCHMGR manages all aspects of searching, including
// UI and the search engine interface.
//
class	SEARCHMGR : public MODEMGR {

    private:

	// Window in which user enters queries.
	//
	INPUTWIN	*querywin;

	// Query/search property sheet.
	//
	QUERYPROPS	*queryprops;

	// Current query.
	//
	QUERY		*curr_query;

	// Query history window.
	//
	QUERYHIST	*queryhist;

	// Search result list (hit list).
	// 
	HITLIST		*hitlist;

	// Button for initiating search.
	//
	Panel_item	search_button;

	// Button for bringing up search history window.
	//
	Panel_item	history_button;

	// Button for bringing up search properties sheet.
	//
	Panel_item	props_button;

	// AnswerBooks with which we're currently dealing.
	//
	ABGROUP		*abgroup;

	// Accessor methods for some SEARCHMGR object members.
	//
	QUERYPROPS	*GetQueryProps();
	QUERYHIST	*GetQueryHist();

	// Perform search operation.
	//
	void		DoSearch();


    public:

	// SEARCHMGR constructor, destructor.
	//
	SEARCHMGR(Xv_opaque frame, NOTIFY *notify, ABGROUP *abgroup_arg);
	~SEARCHMGR();

	// Update search mode window (just clear out any existing hits).
	// 
	STATUS		Update(ERRSTK &err);

	// Make the toc panel and widgets (in)visible.
	// 
	void		Show(BOOL);

	// Get the currently selected document in the hit list.
	// Assumes there is a current selection (see "HasSelection()").
	//
	const DOCNAME	&GetSelection() const;

	// Is there a currently selected document in this hit list?
	//
	BOOL		HasSelection() const;

	// Event dispatcher and handler for search UI events.
	// The dispatcher is a static method that simply passes
	// the event on to the per-object event handler.
	//
	static void	DispatchEvent(	int	event,
					caddr_t	event_obj,
					caddr_t	client_data);
	void		EventHandler(	int	event,
					caddr_t	event_obj);
};


// Events handled by SEARCHMGR.
//
typedef enum {
	DO_SEARCH_EVENT       = 701,	// user initiated search
	SEARCH_PROPS_EVENT    = 702,	// user clicked "Search Props" button
	SEARCH_HISTORY_EVENT  = 703,	// user clicked "Search History" button
	QUERYWIN_RETURN_EVENT = 704,	// user hit RETURN in query window
} SEARCH_EVENT;

#endif	_SEARCHMGR_H
