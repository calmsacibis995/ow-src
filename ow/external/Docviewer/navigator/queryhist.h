#ifndef	_QUERYHIST_H
#define	_QUERYHIST_H

#ident "@(#)queryhist.h	1.18 06/11/93 Copyright 1990-1992 Sun Microsystems, Inc."


// This module defines the interface for the Query History class QUERYHIST.
//

#include "navigator.h"
#include "xview.h"

// Definitions for forward class references.
//
class	INPUTWIN;


class	QUERYHIST {

    private:

	// Text window containing queries.
	//
	INPUTWIN	*histwin;

	// Command frame for history popup.
	//
	Xv_opaque	frame;

	// Number of queries in history list.
	//
	int		nqueries;

	// Current state of this object.
	//
	OBJECT_STATE	objstate;


    public:

	// QUERYHIST constructor, destructor.
	//
	QUERYHIST(Xv_opaque parent);
	~QUERYHIST();

	// Display query history popup.
	//
	void	Show();

	// Add text of a query to the query history window.
	//
	void	AddQuery(const STRING &query);
};

#endif	_QUERYHIST_H
