#ifndef	_BOOKMARKLIST_H
#define	_BOOKMARKLIST_H

#ident "@(#)bookmarklist.h	1.23 06/11/93 Copyright 1990-1992 Sun Microsystems, Inc."


// This module defines the interface for the Bookmark List class BOOKMARKLIST.
//

#include "common.h"
#include "winlist.h"
#include <doc/list.h>


// Forward class declarations.
//
class	BOOKMARK;


class	BOOKMARKLIST : public WINLIST {

    private:

	// Current state of this object.
	//
	OBJECT_STATE	objstate;


    public:

	// BOOKMARKLIST constructor, destructor.
	//
	BOOKMARKLIST(Xv_opaque panel, int x, int y);
	~BOOKMARKLIST(void);

	// Display list of bookmarks in current bookshelf
	// (i.e., "navigator->bookshelf" in this BOOKMARKLIST.
	// Clears list first, if necessary.
	//
	STATUS		Load(ERRSTK &err);

	// Is bookmark list empty?
	//
	BOOL		IsEmpty(void);

	// Add a bookmark to this BOOKMARKLIST.
	//
	void		Add(BOOKMARK *);

	// Get the bookmark corresponding to the specified list entry.
	//
	BOOKMARK	*GetBookmark(int) const;

	void		UpdateEntry(int n);
	void		DeleteEntry(int n);
};

#endif	_BOOKMARKLIST_H
