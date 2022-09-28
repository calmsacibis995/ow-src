#ifndef	_ISAMDOC_H
#define	_ISAMDOC_H

#ident "@(#)isamdoc.h	1.8 06/11/93 Copyright 1989 Sun Microsystems, Inc."


#include <doc/document.h>
#include <doc/isambook.h>


// Forward references.
//
class	ISAMBOOK;
struct	ISAMREC;
class	XXDOC;


class	ISAMDOC : public DOCUMENT {

    private:

	// ISAM-specific fields.
	//
	long		parent_rec;
	long		first_child_rec;
	long		last_child_rec;
	long		next_sibling_rec;
	long		prev_sibling_rec;

	// The book from whence we came.
	//
	ISAMBOOK	*book;

	OBJECT_STATE	objstate;


    public:

	// Construct ISAM document from ISAM database record.
	// Must check object's validity (see "IsValid()") before use.
	//
	ISAMDOC(const ISAMREC *isamrec, ISAMBOOK *book);

	// Construct ISAM document from XXDOC.
	// Must check object's validity (see "IsValid()") before use.
	//
	ISAMDOC(const XXDOC &doc);

	~ISAMDOC();

	// See if this document object is valid.
	//
	BOOL		IsValid();

	BOOL		IsLeaf() const;

	DOCUMENT	*GetParent(ERRSTK &) const;
	DOCUMENT	*GetNextSibling(ERRSTK &) const;
	DOCUMENT	*GetPrevSibling(ERRSTK &) const;
	DOCUMENT	*GetFirstChild(ERRSTK &) const;
	DOCUMENT	*GetLastChild(ERRSTK &) const;

	// Book to which this document belongs.
	//
	const BOOK	*GetBook() const	{ return(book); }
};

// Convert generic XXDOC into ISAM record.
//
STATUS	CvtDocToRec(const XXDOC *, ISAMREC *, ERRSTK &);

// Invalid ISAM record number - useful for initialization.
//
extern const long	ISAM_RECNUM_INVALID;

// ISAM record numbers must be greater than zero.
//
inline BOOL
IsValidRecNum(long recnum)
{
	return(recnum > 0 ? BOOL_TRUE : BOOL_FALSE);
}


#endif	_ISAMDOC_H
