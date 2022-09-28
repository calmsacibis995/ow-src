#ifndef	_DBMBOOK_H
#define	_DBMBOOK_H

#ident "@(#)dbmbook.h	1.9 06/11/93 Copyright 1990 Sun Microsystems, Inc."

#include <doc/book.h>
#include <ndbm.h>
#include <fcntl.h>	// for "Open()" mode, flags


// Forward references
class	DOCUMENT;
class	DBMDOC;
class	XXDOC;


// A DBMBOOK is a named database containing document records.
//
class	DBMBOOK : public BOOK {

    private:

	DBM		*dbhandle;	// ndbm database handle

	// The book constructor is private - the only way to create
	// a book object is via the "Open()" method.
	//
	DBMBOOK(const BOOKNAME	&name,
		const STRING	&path,
		DBM		*handle);

	// Initialize new book database.
	//
	STATUS		Init(ERRSTK &);

	// See if a book is valid.
	//
	BOOL		IsValid(ERRSTK &);

	// Lookup document by its DBM key.
	//
	DBMDOC		*GetDocByKey(datum *key, ERRSTK &);

	OBJECT_STATE	objstate;


    public:

	~DBMBOOK();

	// See if a book exists.
	//
	static BOOL	Exists(const STRING &path);

	// Remove the files comprising the specified book.
	//
	static STATUS	Remove(const STRING &path, ERRSTK &);

	// Open specified book.
	// Returns pointer to new DBMBOOK, or NULL on error.
	// Caller is responsible for deallocation.
	//
	static DBMBOOK	*Open(	const BOOKNAME &name,
				const STRING &path,
				int flags,
				int mode,
				ERRSTK &);

	// Retrieve document by document id
	// Returns new DOCUMENT (caller is responsible for deallocation),
	// or NULL if object id doesn't match.
	//
	DOCUMENT	*GetDocById(const STRING &docid, ERRSTK &);

	// Cycle through all documents in book (in no particular order)
	// Returns new DOCUMENT (caller is responsible for deallocation),
	// or NULL if there are no more documents.
	//
	DOCUMENT	*GetFirstDoc(ERRSTK &);
	DOCUMENT	*GetNextDoc(ERRSTK &);

	// Add new document to book.
	// Fails if document already present in book database.
	//
	STATUS		InsertDoc(const XXDOC *, ERRSTK &);

	// If doc is present in book, replace it.
	// Otherwise, add doc to book.
	//
	STATUS		ReplaceDoc(const XXDOC *, ERRSTK &);

	// If doc is present in book, delete it.
	//
	STATUS		DeleteDoc(const STRING &docid, ERRSTK &);

	// Given object id, determine if that doc is present in book.
	//
	BOOL		DocIsPresent(const STRING &docid);
};

#endif	_DBMBOOK_H
