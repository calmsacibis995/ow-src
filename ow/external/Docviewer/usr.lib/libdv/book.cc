#ident "@(#)book.cc	1.35 93/02/15 Copyright 1989-1992 Sun Microsystems, Inc."

#include <doc/book.h>
#include <doc/isambook.h>
#include <doc/dbmbook.h>
#include "dvlocale.h"


BOOK::BOOK(const BOOKNAME &book_name_arg, const STRING &book_path_arg)
	: book_name(book_name_arg),
	  book_path(book_path_arg),
	  refcount(0)
{
	assert(book_name.IsValid());
	assert(book_path != NULL_STRING);

	objstate.MarkReady();

	DbgFunc("BOOK::BOOK: " << this << endl);
}


BOOK::~BOOK()
{
	DbgFunc("BOOK::~BOOK: " << this << endl);

#ifdef	DEBUG
	if (refcount != 0) {
		cout	<< "BOOK::~BOOK: nonzero reference count for '"
			<< this << "': " << refcount << endl;
	}
#endif	DEBUG
}

// Open specified book *read-only*.
// Returns pointer to new BOOK, or NULL on error.
// Caller is responsible for deallocation.
//
BOOK *
BOOK::Open(	const BOOKNAME &name,
		const STRING &path,
		ERRSTK &err)
{
	DbgFunc("BOOK::Open: "
		<< "name=" 	<< name
		<< "path="	<< path
		<< endl);

	if (ISAMBOOK::Exists(path)) {
		return(ISAMBOOK::Open(name, path, O_RDONLY, 0, err));

	} else if (DBMBOOK::Exists(path)) {
		return(DBMBOOK::Open(name, path, O_RDONLY, 0, err));

	} else {
		err.Init(DGetText("can't open book %s (%s): no such book"),
				~name, ~path);
		return(NULL);
	}
}

// See if this book exists.
//
BOOL
BOOK::Exists(const STRING &path)
{
	DbgFunc("BOOK::Exists: " << path << endl);

	if (ISAMBOOK::Exists(path)) {
		return(BOOL_TRUE);

	} else if (DBMBOOK::Exists(path)) {
		return(BOOL_TRUE);

	} else {
		return(BOOL_FALSE);
	}
}

// Increment, decrement reference count.
//
void
BOOK::AddRef()
{
	assert(objstate.IsReady());

	++refcount;

	DbgFunc("BOOK::AddRef:" << Name() << " (" << refcount << ")" << endl);
}

void
BOOK::DeleteRef()
{
	assert(objstate.IsReady());

	--refcount;
	assert(refcount >= 0);

	DbgFunc("BOOK::DeleteRef:" << Name() << " (" << refcount << ")"<<endl);
}

int
BOOK::RefCount() const
{
	assert(objstate.IsReady());
	DbgFunc("BOOK::RefCount:" << Name() << " (" << refcount << ")" <<endl);

	return(refcount);
}

// Print book info.
//
ostream &
operator << (ostream &ostr, BOOK *book)
{
	if (book != NULL)
		ostr << book->Name() << " (" << book->Path() << ")";

	return(ostr);
}
