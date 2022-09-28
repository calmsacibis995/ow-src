#ifndef	_BOOK_H
#define	_BOOK_H

#ident "@(#)book.h	1.26 06/11/93 Copyright 1990 Sun Microsystems, Inc."


#include <doc/common.h>
#include <doc/bookname.h>


// Forward references
//
class	DOCUMENT;
class	XXDOC;


// A BOOK is a named database containing document records.
//
class	BOOK {

    private:

	BOOKNAME	book_name;	// book's object name
	STRING		book_path;	// pathname of database

	// The book caching mechanism needs to know whether
	// a given book is currently being referenced by
	// DOCUMENT objects in order to determine which books
	// can safely be deleted.
	//
	int		refcount;

	OBJECT_STATE	objstate;	// current state of this book


    protected:

	// Book constructor.
	// Note that it is only accessible to objects derived from BOOK
	// (e.g., ISAMBOOK, DBMBOOK) because we want to force everyone
	// to get create new BOOK objects via BOOK::Open().
	//
	BOOK(const BOOKNAME &name, const STRING &path);


    public:

	// Book destructor.
	// Note that this destructor must be virtual in order for objects
	// derived from BOOK (e.g., ISAMBOOK, DBMBOOK) to be
	// properly cleaned up.
	// Note that this implies that BOOK objects mustn't require
	// any cleanup of its own, since there is no real BOOK destructor.
	//
	virtual		~BOOK();

	// See if this book exists.
	//
	static BOOL	Exists(const STRING &path);

	// Open specified book *read-only*.
	// Returns pointer to new BOOK, or NULL on error.
	// Caller is responsible for deallocation.
	//
	static BOOK	*Open(	const BOOKNAME &name,
				const STRING &path,
				ERRSTK &err);

	// Return this book's book object name, id, path, language.
	//
	const BOOKNAME	&Name() const	{ return(book_name); }
	const STRING	&Id() const	{ return(book_name.BookId()); }
	const STRING	&Lang() const	{ return(book_name.BookLang()); }
	const STRING	&Path() const	{ return(book_path); }

	// Increment, decrement reference count.
	//
	void		AddRef();
	void		DeleteRef();
	int		RefCount() const;

	// Retrieve document by object id
	// Returns new DOCUMENT (caller is responsible for deallocation),
	// or NULL if object id doesn't match.
	//
	virtual DOCUMENT	*GetDocById(const STRING &, ERRSTK&) = 0;

	// Cycle through all documents in book (in no particular order)
	// Returns new DOCUMENT (caller is responsible for deallocation),
	// or NULL if there are no more documents.
	//
	virtual DOCUMENT	*GetFirstDoc(ERRSTK&) = 0;
	virtual DOCUMENT	*GetNextDoc(ERRSTK&) = 0;

	// Add new document to book.
	// Assumes doc's id is unique (i.e., doc not already in book).
	//
	virtual STATUS		InsertDoc(const XXDOC *, ERRSTK &) = 0;

	// If doc is present in book, replace it.
	// Otherwise, add doc to book.
	//
	virtual STATUS		ReplaceDoc(const XXDOC *, ERRSTK &) = 0;

	// If doc is present in book, delete it.
	//
	virtual STATUS		DeleteDoc(const STRING &, ERRSTK &) = 0;

	// Given object id, determine if that doc is present in book.
	//
	virtual BOOL		DocIsPresent(const STRING &) = 0;

	// Print book info.
	//
	friend ostream	&operator << (ostream &, BOOK *);

	// Timestamp for use by BOOKSHELF book caching mechanism.
	//
	int			timestamp;
};

#endif	_BOOK_H
