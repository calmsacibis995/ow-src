#ifndef	_ISAMBOOK_H
#define	_ISAMBOOK_H

#ident "@(#)isambook.h	1.10 06/11/93 Copyright 1990 Sun Microsystems, Inc."

#include <doc/book.h>
#include <fcntl.h>	// for "Open()" mode, flags


// Forward references.
//
class	XXDOC;
class	DOCUMENT;
struct	ISAMREC;


// An ISAMBOOK is a named ISAM database containing document records.
//
class	ISAMBOOK : public BOOK {

    private:

	int		isam_fd;	// database file handle

	// The book constructor is private - the only way to create
	// a book object is via the "Open()" method.
	//
	ISAMBOOK(const BOOKNAME &name, const STRING &path, int isfd);

	// Flush updates to ISAM book database.
	//
	STATUS		Flush(ERRSTK &);

	OBJECT_STATE	objstate;


    public:

	// Close ISAM book database (if currently open).
	// Clean up.
	//
	~ISAMBOOK();

	// See if this book exists.
	//
	static BOOL	Exists(const STRING &path);

	// Remove the files comprising the specified book.
	//
	static STATUS	Remove(const STRING &path, ERRSTK &);

	// Open specified book.
	// Returns pointer to new ISAMBOOK, or NULL on error.
	// Caller is responsible for deallocation.
	//
	static ISAMBOOK	*Open(	const BOOKNAME &name,
				const STRING &path,
				int flags,
				int mode,
				ERRSTK &);

	// Retrieve document by document id
	// Returns new DOCUMENT (caller is responsible for deallocation),
	// or NULL if object id doesn't match.
	//
	DOCUMENT	*GetDocById(const STRING &docid, ERRSTK &);

	// Retrieve document by record number.
	// Returns new DOCUMENT (caller is responsible for deallocation),
	// or NULL on error.
	//
	DOCUMENT	*GetDocByRecNum(long recnum, ERRSTK &);

	// Cycle through all documents in book (in no particular order)
	// Returns new DOCUMENT (caller is responsible for deallocation),
	// or NULL if there are no more documents.
	//
	DOCUMENT	*GetFirstDoc(ERRSTK &);
	DOCUMENT	*GetNextDoc(ERRSTK &);

	// Add new document to book.
	// Assumes doc's id is unique (i.e., doc not already in book).
	//
	STATUS		InsertDoc(const XXDOC *, ERRSTK &);

	// If doc is present in book, replace it.
	// Otherwise, add doc to book.
	//
	STATUS		ReplaceDoc(const XXDOC *, ERRSTK &);

	// If doc is present in book, delete it.
	// Returns STATUS_OK if doc is successfully deleted
	// OR is not there to begin with, else returns STATUS_FAILED.
	//
	STATUS		DeleteDoc(const STRING &docid, ERRSTK &);

	// Given object id, determine if that doc is present in book.
	//
	BOOL		DocIsPresent(const STRING &docid);
};

#endif	_ISAMBOOK_H
