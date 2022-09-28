#ifndef	_CARDCATS_H
#define	_CARDCATS_H

#ident "@(#)cardcats.h	1.8 06/22/93 Copyright 1992 Sun Microsystems, Inc."


#include <doc/common.h>
#include <doc/abname.h>
#include <doc/listx.h>

// The CARDCATS object is simply a convenient way of maintaining and using
// a list of card catalogs (CARDCAT objects).
//
// Typically when we search for the ABINFO object for a particular AnswerBook,
// we'll want to look for it in one of several card catalogs.  That's
// where CARDCATS comes in handy.
//
// Note that CARDCATS provides a query-only interface to card catalogs.
// If you want to create or update a card catalog, use CARDCAT.
//
// CARDCATS can be set up to use the default card catalogs, which are
// (in the order in which they're searched):
//
//	$CARD_CATALOG
//	$HOME/.card_catalog
//	/etc/card_catalog
//


// Forward references.
//
class	ABINFO;
class	CARDCAT;


class	CARDCATS {

    private:

	// List of card catalogs we're managing.
	// Note that this is an auto-delete list (LISTX).
	//
	LISTX<CARDCAT*>	cclist;

	// Current state of this object.
	//
	OBJECT_STATE	objstate;

	// Insert the specified card catalog into our list.
	// Note that "ccpath" is resolved to a full path name -
	// this is important because these path names tend to get passed
	// between processes that may have different views of the environment.
	//
	STATUS		Insert(const STRING &ccpath, int &n, ERRSTK &err);


    public:

	// Card catalog constructor, destructor.
	//
	CARDCATS();
	~CARDCATS();

	// Add the default card catalogs to the end of this list.
	//
	STATUS		AppendDefaults(ERRSTK &err);

	// Append/prepend the specified list of card catalogs to our list.
	// "cclist" is a colon-separated list of card catalog paths.
	// Note that each card catalog path name in "ccpaths" is resolved
	// to a full path name.
	//
	STATUS		Append(const STRING &ccpaths, ERRSTK &err);
	STATUS		Prepend(const STRING &ccpaths, ERRSTK &err);

	// Clear out this list of card catalogs.
	//
	void		Clear();

	// Retrieve the ABINFO record for the specified AnswerBook.
	// Search each of the card catalogs in our list in order.
	//
	STATUS		GetMatch(const ABNAME &abname, ABINFO &info, ERRSTK &);

	// Retrieve the first/next ABINFO record in this card catalog.
	//
	STATUS		GetFirst(ABINFO &info, ERRSTK &);
	STATUS		GetNext(ABINFO &info, ERRSTK &);

	// Retrieve all ABINFO records in every card catalog in this list.
	// Records in list are all new ABINFOs (caller is responsible
	// for deallocation),
	// Clears info_list before starting.
	//
	STATUS		GetAll(LISTX<ABINFO*> &info_list, ERRSTK &);

	// Get the path names for the card catalogs in this list.
	// "paths" argument can either be a list of strings,
	// or a single string (in which case "paths" will contain
	// a colon-separated list, handy for passing to other processes, e.g.).
	//
	void		GetPaths(LIST<STRING> &paths);
	void		GetPaths(STRING &paths);
};

#endif	_CARDCATS_H
