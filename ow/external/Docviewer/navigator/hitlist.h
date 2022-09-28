#ifndef	_HITLIST_H
#define	_HITLIST_H

#ident "@(#)hitlist.h	1.46 93/02/10 Copyright 1990-1992 Sun Microsystems, Inc."

// This module defines the interface for the user interface search result 
// presentation class called HITLIST. The HITLIST class is derived form the
// generic navigator window list class (WINLIST) . The only member functions 
// defined are for HITLIST specific UI behavior.

#include "navigator.h"
#include "winlist.h"
#include <doc/docname.h>
#include <doc/list.h>
#include <doc/listx.h>
#include <doc/query.h>
#include <doc/searchdoc.h>


// Forward references
//
class	ABGROUP;
class	HIT;


class	HITLIST : public WINLIST {

    private:

	// List of documents found as result of search.
	//
	LISTX<HIT*>	hitlist;

	// AnswerBooks with which we're currently dealing.
	//
	ABGROUP		*abgroup;

	// Current state of this object.
	//
	OBJECT_STATE	objstate;

	// Sort hitlist according to specified sort order.
	//
	static void	SortHitList(LISTX<HIT*>		&hitlist,
				    SORT_SEARCH_RESULTS	order);

	// Comparison routines for sorting search hitlist.
	//
	static int	SortHitListByRank(const void *, const void *);
	static int	SortHitListByBook(const void *, const void *);


    public:

	// HITLIST constructor/destructor and initialization routines.
	// Note that 'Init()' must be called after a HITLIST object
	// is instantiated but before it is used.
	//
	HITLIST(Xv_opaque panel, int x, int y, ABGROUP *abgroup_arg);
	~HITLIST();

	// Clear hits from the hitlist.
	//
	void		Clear();

	// Display the search results
	// attached to a particular query.
	//
	STATUS		Display(LIST<SEARCHDOC*>	&results,
				SORT_SEARCH_RESULTS	sort_order,
				ERRSTK			&err);

	// Get the DOCNAME of the document corresponding to
	// the specified entry in this Location list.
	//
	const DOCNAME	&GetDocName(int n);

	// Get current number of documents in this hit list.
	//
	int		NumHits();
};

#endif	_HITLIST_H
