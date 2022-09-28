#ifndef	_DOCID_H
#define	_DOCID_H

#ident "@(#)docid.h	1.11 06/11/93 Copyright 1990 Sun Microsystems, Inc."


#include <doc/common.h>


// Routines for manipulating document id's.
//
// A Document id is just a STRING that, by convention, has two parts:
//	- a book id
//	- and a unique key (unique to the particular book)
//


// Get the book id (key) part of the doc id.
//
// Returns null string if there's no book id (key) part.
//
const STRING	GetBookIdFromDocId(const STRING &docid);
const STRING	GetKeyFromDocId(const STRING &docid);


// Make doc id from book id + key.
//
const STRING	MakeDocId(const STRING &bookid, const STRING &key);

// Is this a valid doc id?
//
BOOL		IsValidDocId(const STRING &docid);


// Is this a root doc id?
//
BOOL		IsRootDocId(const STRING &docid);

#endif	_DOCID_H
