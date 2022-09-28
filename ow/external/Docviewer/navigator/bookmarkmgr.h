#ifndef	_BOOKMARKMGR_H
#define	_BOOKMARKMGR_H

#ident "@(#)bookmarkmgr.h	1.24 96/11/15 Copyright 1990-1992 Sun Microsystems, Inc."

#include "common.h"
#include "modemgr.h"
#include "xview.h"
#include <doc/list.h>
#include <doc/docname.h>

// Forward class references.
//
class	ABGROUP;
class	BOOKMARK;
class	BOOKMARKEDIT;
class	BOOKMARKLIST;
class	INPUTWIN;


// Events handled by BOOKMARKMGR.
//
typedef enum {

	// Create new bookmark.
	BM_NEW_EVENT		= 1201,

	// Delete currently selected bookmark
	BM_DELETE_EVENT		= 1202,

	// Modify currently selected bookmark
	BM_MODIFIED_EVENT	= 1203,

	// Save changes to currently selected bookmark
	BM_SAVE_CHANGES_EVENT	= 1204
} BOOKMARK_EVENT;


//
//
class	BOOKMARKMGR : public MODEMGR {

    private:

	BOOKMARKLIST	*bmlist;

	// Bookmark window.
	// UI component for creating bookmarks,
	// and viewing and editing bookmark annotations.
	//
	INPUTWIN	*bmwin;

	// Bookmark creation window.
	//
	BOOKMARKEDIT	*bmnew;

	// The current bookmark.
	//
	BOOKMARK	*curr_bookmark;

	// Has user edited the current bookmark?
	//
	BOOL		bookmark_is_modified;

	// Bookmark panel widgets.
	//
	Panel_item	delete_button;
	Panel_item	save_button;
	Panel_item	title_widget;

	// Group of AnswerBooks with which we're currently working.
	//
	ABGROUP		*abgroup;

	// Modify UI as necessary to reflect whether bookmark list
	// is empty or non-empty.
	//
	void		NoBookmarks();
	void		SomeBookmarks();

	// Handle setting of a new "current bookmark".
	// If "save_edits" is true, save any outstanding edits to the
	// existing current bookmark.
	// Prompt user before saving the edits.
	// "new_bookmark" can be NULL iff the bookmark list is
	// completely empty.
	//
	typedef enum { SAVE_CHANGES, DISCARD_CHANGES } CHANGES;
	void		SetCurrentBookmark(	BOOKMARK	*new_bookmark,
						CHANGES		changes);

	// Event handlers for individual BOOKMARKMGR events.
	// This is where all the work gets done.
	//
	void		SaveEvent();
	void		SaveChangesEvent();
	void		CancelEvent();
	void		DeleteEvent();
	void		SelectEvent();

	
    public:

	// BOOKMARKMGR constructor, destructor.
	//
	BOOKMARKMGR(Xv_opaque frame, NOTIFY *notify, ABGROUP *abgroup_arg);
	~BOOKMARKMGR();

	// The bookmark list has changed (e.g., there is a new list).
	// Reload bookmark list.
	// 
	STATUS		Update(ERRSTK &err);

	// Make the toc panel and widgets (in)visible.
	// 
	void		Show(BOOL);

	// Make sure the scrolling list knows how many items to contain
	//
	void		ResizeEvent();

	// Create a new bookmark to the specified document.
	//
	void		CreateBookmark(const DOCNAME &docname);

	// Get the currently selected document in the bookmark list.
	// Assumes there is a current selection (see "HasSelection()").
	//
	const DOCNAME	&GetSelection() const;

	// Is there a currently selected document in this bookmark list?
	//
	BOOL		HasSelection() const;

	// Main event handler.
	//
	void		EventHandler(int event, caddr_t event_obj);

	// Event dispatcher for Bookmark Creation window, Bookmark List events.
	// Simply passes event on to "EventHandler()".
	// This method is declared "static" because it is used
	// as a callback in an environment where its class is not known.  
	//
	static void	DispatchEvent(	int	event,
					caddr_t	event_obj,
					caddr_t	client_data);
};

#endif	_BOOKMARKMGR_H
