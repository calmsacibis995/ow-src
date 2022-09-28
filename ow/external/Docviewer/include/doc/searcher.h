#ifndef	_SEARCHER_H
#define	_SEARCHER_H

#ident "@(#)searcher.h	1.11 06/11/93 Copyright 1990-1992 Sun Microsystems, Inc."

// This file contains the interface definition for the SEARCHER class.
// Searcher manages the application interface to the Ful/Text
// search and retrieval API.  The current Ful/Text API is restricted to
// a single thread (session) per application due to low level operations
// that are currently not implemented in a reentrant fashion.
// There should be one and only one SEARCHER active per application at a time.
//

#define FTLINT
#define FTHANSIC

extern "C" {
#include <ft/ftdefs.h>
}

#include <doc/searchdoc.h>
#include <doc/list.h>
#include <doc/listx.h>

// Forward references.
//
class	QUERY;
class	SCOPEKEY;
class	ZONEWGHT;


class	SEARCHER {

    private:

	// Handle for Ful/Text API.
	// Only one per customer, please (that's why it's "static").
	//
	static FTAPIH	api_handle;

	// How many SEARCHERs are referencing the api_handle?
	// We get rid of the handle once the reference count drops to zero.
	//
	static int	api_refcount;

	// F/T configuration info for this collection.
	//
	FTFIG		config;

	// F/T collection handle (returned by 'ftbopen()').
	//
	FTBH		coll_handle;

	// F/T catalog handle for this collection (returned by 'ftcopen()').
	//
	FTCH		cat_handle;

	// Full path name of this collection.
	//
	STRING		index_path;

	// Current state of this object.
	//
	OBJECT_STATE	objstate;

	// Read record from collection's catalog.
	//
	STATUS		GetCatalogEntry(FTCID cid, FTCINFO *, ERRSTK &);

	// Initialize Ful/Text API.
	//
	STATUS		InitFT(ERRSTK &);

	// Initialize Scoping Keys and Zone Weights
	// for this collection.
	//
	STATUS		InitKeys(ERRSTK &);
	STATUS		InitWeights(ERRSTK &);

	// Perform first half of search operation.
	//
	STATUS		DoSearch(	const STRING		&query,
					int			max_docs,
					FTSH			&search,
					ERRSTK			&err);

	// Perform second half of search operation.
	//
	STATUS		GetResults(	FTSH			search,
					LISTX<SEARCHDOC*>	&results,
					ERRSTK			&err);


    public:

	// SEARCHER constructor, destructor.
	//
	SEARCHER(const STRING &ndxpath);
	~SEARCHER();

	// Initialize SEARCHER object
	//
	STATUS		Init(ERRSTK &);

	// Perform search operation on this collection.
	//
	STATUS		Search(	const QUERY		&query,
				int			max_docs,
				LISTX<SEARCHDOC*>	&results,
				ERRSTK			&err);

	// List of scoping keys for the current Ful/Text collection.
	// Scope keys allow a given query to search particular subset(s)
	// of the collection.
	//
	LISTX<SCOPEKEY*> keys;

	// List of zone weights for this collection.
	// Used by search engine in relevance rank computations.
	//
	LISTX<ZONEWGHT*> weights;

	// Get path name of this collection.
	//
	const STRING	&IndexPath()	{ return(index_path); }

	// Handy routine for printing SEARCHER info.
	//
	friend ostream	&operator << (ostream &, SEARCHER *);
};

#endif	_SEARCHER_H
